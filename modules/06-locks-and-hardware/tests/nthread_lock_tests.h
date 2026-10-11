// Shared tests for an n-thread lock with lock(i)/unlock(i), i in 0..n-1.
// Define LOCK_INIT(n), LOCK(i), UNLOCK(i) and LOCK_NAME before including.
#include "harness.h"

static harness_cs_monitor mon = HARNESS_CS_MONITOR_INIT;
static long counter;          // plain long: only safe if the lock works
static long iters;
static long total_entries;

static void contended_worker(int id, void *ctx) {
    (void)ctx;
    for (long k = 0; k < iters; k++) {
        LOCK(id);
        harness_cs_enter(&mon);
        long tmp = counter;   // a deliberately slow read-modify-write
        if ((k & 255) == 0) harness_jitter();
        counter = tmp + 1;
        harness_cs_exit(&mon);
        UNLOCK(id);
    }
}

// A thread alone must get in every time, for every id.
static void test_each_thread_alone(void) {
    LOCK_INIT(4);
    for (int id = 0; id < 4; id++)
        for (int k = 0; k < 200; k++) {
            LOCK(id);
            UNLOCK(id);
        }
    CHECK(1, "unreachable");
}

static void run_contended(int n, long base_iters) {
    LOCK_INIT(n);
    counter = 0;
    iters = harness_iters(base_iters);
    long before = atomic_load(&mon.violations);
    harness_parallel(n, contended_worker, NULL);
    long v = atomic_load(&mon.violations) - before;
    CHECK(v == 0, "%d threads: two threads were inside the critical section together %ld times",
          n, v);
    CHECK(counter == n * iters, "%d threads: counter == %ld, expected %ld (lost updates mean "
          "two threads were inside together)", n, counter, n * iters);
    total_entries += n * iters;
}

static void test_two_threads(void)   { run_contended(2, 50000); }
static void test_four_threads(void)  { run_contended(4, 20000); }
// More threads than many laptops have cores: waiting threads must not stop
// the lock holder from running (spin_pause gives the CPU away now and then).
static void test_eight_threads(void) { run_contended(8, 5000); }

static int run_nthread_lock_tests(void) {
    harness_watchdog(50);
    harness_hint("Use LOAD/STORE on every shared variable and call spin_pause() in every waiting "
                 "loop. A timeout means some thread waits for something that never happens.");
    RUN_TEST(test_each_thread_alone);
    RUN_TEST(test_two_threads);
    RUN_TEST(test_four_threads);
    RUN_TEST(test_eight_threads);
    if (atomic_load(&mon.violations) == 0)
        printf("  (no violation observed in %ld contended entries - a stress test cannot prove "
               "correctness; tools/interleave can)\n", total_entries);
    return harness_report();
}
