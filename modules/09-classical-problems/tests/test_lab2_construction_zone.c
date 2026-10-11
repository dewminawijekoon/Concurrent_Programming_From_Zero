// TIMEOUT: 60
// Tests for lab track 2 (Lab 04 Ex 2): one direction, capacity N, and fair.
void zone_init(int n);
void zone_destroy(void);
void zone_enter(int type);
void zone_exit(int type);
#define BR_INIT(c) zone_init(c)
#define BR_DESTROY() zone_destroy()
#define BR_ENTER(d) zone_enter(d)
#define BR_EXIT(d) zone_exit(d)
#define BR_NAME_0 "supply"
#define BR_NAME_1 "return"
#include "bridge_tests.h"

// Supply truck S1 inside. Return truck R waits. Then supply truck S2 arrives.
// S2 must not overtake R, or a stream of supply trucks would starve R.
static atomic_int r_in, s2_in;
static void *return_truck(void *arg) {
    (void)arg;
    zone_enter(1);
    atomic_store(&r_in, 1);
    return NULL;
}
static void *supply_truck(void *arg) {
    (void)arg;
    zone_enter(0);
    atomic_store(&s2_in, 1);
    return NULL;
}
static void test_no_overtaking_a_waiting_truck(void) {
    zone_init(3);
    atomic_store(&r_in, 0);
    atomic_store(&s2_in, 0);
    zone_enter(0);                                     // S1 inside
    pthread_t r, s2;
    pthread_create(&r, NULL, return_truck, NULL);      // R waits for S1
    sleep_ms(100);
    pthread_create(&s2, NULL, supply_truck, NULL);     // S2 arrives after R
    sleep_ms(100);
    int overtook = atomic_load(&s2_in);
    CHECK(!overtook, "supply truck S2 arrived after return truck R was waiting, but went in first. "
                     "With a steady stream of supply trucks, R would wait forever (starvation)");
    zone_exit(0);                                      // S1 leaves
    if (overtook) zone_exit(0);                        // let S2 out too, so R can finish
    int r_got_in = wait_for_flag(&r_in, 2000);
    CHECK(r_got_in, "the zone emptied, but the waiting return truck never got in");
    if (r_got_in && !overtook) {
        sleep_ms(50);
        CHECK(!atomic_load(&s2_in), "supply truck S2 entered while return truck R was inside");
        zone_exit(1);                                  // R leaves
        CHECK(wait_for_flag(&s2_in, 2000), "R left, but supply truck S2 never got in");
        zone_exit(0);
    } else if (r_got_in) {
        zone_exit(1);
    }
    pthread_join(r, NULL);
    pthread_join(s2, NULL);
    zone_destroy();
}

int main(void) {
    harness_watchdog(50);
    harness_hint("Lab track 1's design plus a turnstile: enter = down(turnstile); lightswitch lock; "
                 "up(turnstile); down(capacity). exit = up(capacity); lightswitch unlock.");
    RUN_TEST(test_opposite_direction_waits);
    RUN_TEST(test_same_direction_shares_up_to_capacity);
    RUN_TEST(test_capacity_one);
    RUN_TEST(test_no_overtaking_a_waiting_truck);
    RUN_TEST(test_stress_capacity_3);
    RUN_TEST(test_stress_capacity_1);
    RUN_TEST(test_stress_capacity_5);
    return harness_report();
}
