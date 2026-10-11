// TIMEOUT: 90
#include "harness.h"

void tournament_init(void);
void tournament_lock(int t);
void tournament_unlock(int t);

static harness_cs_monitor mon = HARNESS_CS_MONITOR_INIT;
static long counter, iters;

static void worker(int id, void *ctx) {
    (void)ctx;
    for (long k = 0; k < iters; k++) {
        tournament_lock(id);
        harness_cs_enter(&mon);
        long tmp = counter;
        if ((k & 511) == 0) harness_jitter();
        counter = tmp + 1;
        harness_cs_exit(&mon);
        tournament_unlock(id);
    }
}

static void test_each_thread_alone(void) {
    tournament_init();
    for (int t = 0; t < 4; t++)
        for (int k = 0; k < 100; k++) {
            tournament_lock(t);
            tournament_unlock(t);
        }
    CHECK(1, "unreachable");
}

static void test_four_threads(void) {
    tournament_init();
    counter = 0;
    iters = harness_iters(50000);
    harness_parallel(4, worker, NULL);
    CHECK(atomic_load(&mon.violations) == 0, "%ld mutual-exclusion violations",
          atomic_load(&mon.violations));
    CHECK_EQ(counter, 4 * iters);
}

int main(void) {
    harness_watchdog(60);
    harness_hint("Each node is an ordinary Peterson lock; the side is the child you came from. "
                 "Unlock in reverse order: root first.");
    RUN_TEST(test_each_thread_alone);
    RUN_TEST(test_four_threads);
    return harness_report();
}
