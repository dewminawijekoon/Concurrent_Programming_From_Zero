// Shared tests for any 2-thread lock with lock(i)/unlock(i), i in {0,1}.
// Define LOCK_INIT, LOCK(i), UNLOCK(i) before including.
#include "harness.h"

static harness_cs_monitor mon = HARNESS_CS_MONITOR_INIT;
static long counter;          // plain long: only safe if the lock works
static long iters;

static void contended_worker(int id, void *ctx) {
    (void)ctx;
    for (long k = 0; k < iters; k++) {
        LOCK(id);
        harness_cs_enter(&mon);
        long tmp = counter;   // a deliberately slow read-modify-write
        if ((k & 1023) == 0) harness_jitter();
        counter = tmp + 1;
        harness_cs_exit(&mon);
        UNLOCK(id);
    }
}

// Both threads hammer the lock. Any overlap shows up as a violation or a lost update.
static void test_mutual_exclusion_under_contention(void) {
    LOCK_INIT();
    counter = 0;
    iters = harness_iters(200000);
    harness_parallel(2, contended_worker, NULL);
    CHECK(atomic_load(&mon.violations) == 0,
          "two threads were inside the critical section together %ld times",
          atomic_load(&mon.violations));
    CHECK_EQ(counter, 2 * iters);
}

// One thread alone must get in every time ("success in the absence of
// contention"). A LockTwo-style solution hangs here and the watchdog fires.
static void test_works_without_contention(void) {
    LOCK_INIT();
    for (int id = 0; id < 2; id++)
        for (int k = 0; k < 1000; k++) {
            LOCK(id);
            UNLOCK(id);
        }
    CHECK(1, "unreachable");
}

// Strict alternation (Attempt 1) would hang here: thread 0 enters twice in a row.
static void test_no_strict_alternation(void) {
    LOCK_INIT();
    LOCK(0); UNLOCK(0);
    LOCK(0); UNLOCK(0);
    LOCK(1); UNLOCK(1);
    LOCK(1); UNLOCK(1);
    CHECK(1, "unreachable");
}

static int run_two_thread_lock_tests(void) {
    harness_watchdog(30);
    harness_hint("Use LOAD/STORE on the shared variables, and make sure unlock() undoes what "
                 "lock() announced. A timeout means a thread waits for something that never happens.");
    RUN_TEST(test_works_without_contention);
    RUN_TEST(test_no_strict_alternation);
    RUN_TEST(test_mutual_exclusion_under_contention);
    if (atomic_load(&mon.violations) == 0)
        printf("  (no violation observed in %ld contended entries - a stress test cannot prove "
               "correctness; tools/interleave can)\n", 2 * iters);
    return harness_report();
}
