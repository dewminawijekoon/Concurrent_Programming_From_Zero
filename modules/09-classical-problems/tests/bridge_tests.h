// Shared tests for lab tracks 1 and 2 (one-direction bridge with a capacity).
// Define BR_INIT(cap), BR_DESTROY(), BR_ENTER(dir), BR_EXIT(dir), BR_NAME_0,
// BR_NAME_1 before including.
#include "classic_test_util.h"

static atomic_int helper_in;
static int helper_dir;
static void *helper(void *arg) {
    (void)arg;
    BR_ENTER(helper_dir);
    atomic_store(&helper_in, 1);
    return NULL;   // stays on the bridge; the test calls BR_EXIT for it
}
static pthread_t start_helper(int dir) {
    pthread_t t;
    atomic_store(&helper_in, 0);
    helper_dir = dir;
    pthread_create(&t, NULL, helper, NULL);
    return t;
}

static void test_opposite_direction_waits(void) {
    BR_INIT(3);
    BR_ENTER(0);
    pthread_t t = start_helper(1);
    sleep_ms(100);
    CHECK(!atomic_load(&helper_in), "a %s vehicle entered while a %s vehicle was inside",
          BR_NAME_1, BR_NAME_0);
    BR_EXIT(0);
    int in = wait_for_flag(&helper_in, 2000);
    CHECK(in, "the %s vehicle left, but the waiting %s vehicle never got in", BR_NAME_0, BR_NAME_1);
    pthread_join(t, NULL);
    BR_EXIT(1);
    BR_DESTROY();
}

static void test_same_direction_shares_up_to_capacity(void) {
    BR_INIT(3);
    BR_ENTER(0);   // one thread may enter several times: each call is one vehicle
    BR_ENTER(0);
    BR_ENTER(0);
    pthread_t t = start_helper(0);
    sleep_ms(100);
    CHECK(!atomic_load(&helper_in), "capacity is 3, but a 4th vehicle got in");
    BR_EXIT(0);
    CHECK(wait_for_flag(&helper_in, 2000), "a place was freed, but the 4th vehicle never got in");
    pthread_join(t, NULL);
    BR_EXIT(0);
    BR_EXIT(0);
    BR_EXIT(0);
    BR_DESTROY();
}

static void test_capacity_one(void) {
    BR_INIT(1);
    BR_ENTER(1);
    pthread_t t = start_helper(1);
    sleep_ms(100);
    CHECK(!atomic_load(&helper_in), "capacity is 1, but a 2nd vehicle got in");
    BR_EXIT(1);
    CHECK(wait_for_flag(&helper_in, 2000), "the bridge was freed, but the waiting vehicle never got in");
    pthread_join(t, NULL);
    BR_EXIT(1);
    BR_DESTROY();
}

// ---- stress: many vehicles both ways, check the rules on every entry ----
static atomic_int on_bridge[2], max_seen;
static atomic_long mixed_dirs, over_cap, crossings;
static int cap_now;
static long trips;
static void vehicle(int id, void *ctx) {
    (void)ctx;
    int d = id % 2;
    for (long k = 0; k < trips; k++) {
        harness_jitter();
        BR_ENTER(d);
        int now = atomic_fetch_add(&on_bridge[d], 1) + 1;
        atomic_max(&max_seen, now);
        if (atomic_load(&on_bridge[1 - d]) != 0) atomic_fetch_add(&mixed_dirs, 1);
        if (now > cap_now) atomic_fetch_add(&over_cap, 1);
        harness_jitter();
        atomic_fetch_sub(&on_bridge[d], 1);
        BR_EXIT(d);
        atomic_fetch_add(&crossings, 1);
        atomic_fetch_add(&progress, 1);
    }
}
static void report_bridge(void) {
    printf("          on the bridge right now: %d %s, %d %s\n", atomic_load(&on_bridge[0]), BR_NAME_0,
           atomic_load(&on_bridge[1]), BR_NAME_1);
}
static void stress(int cap) {
    cap_now = cap;
    trips = harness_iters(3000);
    atomic_store(&mixed_dirs, 0);
    atomic_store(&over_cap, 0);
    atomic_store(&crossings, 0);
    atomic_store(&max_seen, 0);
    BR_INIT(cap);
    progress_watch_start(5, "no vehicle moved (deadlock?)", report_bridge);
    harness_parallel(12, vehicle, NULL);
    progress_watch_stop();
    BR_DESTROY();
    CHECK(atomic_load(&mixed_dirs) == 0, "capacity %d: both directions were on the bridge together %ld times",
          cap, atomic_load(&mixed_dirs));
    CHECK(atomic_load(&over_cap) == 0, "capacity %d: more than %d vehicles were on the bridge %ld times",
          cap, cap, atomic_load(&over_cap));
    CHECK(atomic_load(&crossings) == 12 * trips, "only %ld of %ld crossings finished",
          atomic_load(&crossings), 12 * trips);
    if (atomic_load(&mixed_dirs) == 0 && atomic_load(&over_cap) == 0)
        printf("  (capacity %d: %ld crossings, never mixed, at most %d at once - no violation observed)\n",
               cap, 12 * trips, atomic_load(&max_seen));
}
static void test_stress_capacity_3(void) { stress(3); }
static void test_stress_capacity_1(void) { stress(1); }
static void test_stress_capacity_5(void) { stress(5); }
