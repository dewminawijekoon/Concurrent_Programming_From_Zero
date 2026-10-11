// TIMEOUT: 60
#include "ex4_spinlocks.h"
#include "bench.h"
#include "harness.h"

static spinlock lk;
static harness_cs_monitor mon = HARNESS_CS_MONITOR_INIT;
static long counter, iters;
static int use_ttas;

static void lock_it(void)   { if (use_ttas) ttas_lock(&lk); else tas_lock(&lk); }
static void unlock_it(void) { if (use_ttas) ttas_unlock(&lk); else tas_unlock(&lk); }

static void worker(int id, void *ctx) {
    (void)id; (void)ctx;
    for (long k = 0; k < iters; k++) {
        lock_it();
        harness_cs_enter(&mon);
        long tmp = counter;
        if ((k & 255) == 0) harness_jitter();
        counter = tmp + 1;
        harness_cs_exit(&mon);
        unlock_it();
    }
}

static void check_alone(int ttas) {
    use_ttas = ttas;
    spin_init(&lk);
    for (int k = 0; k < 1000; k++) {   // unlock must really free the lock,
        lock_it();                     // or the second lock() spins forever
        unlock_it();
    }
    CHECK(1, "unreachable");
}

static void check_contended(int ttas, int n) {
    use_ttas = ttas;
    spin_init(&lk);
    counter = 0;
    iters = harness_iters(100000);
    long before = atomic_load(&mon.violations);
    harness_parallel(n, worker, NULL);
    long v = atomic_load(&mon.violations) - before;
    CHECK(v == 0, "%s, %d threads: two threads were inside the critical section together %ld times",
          ttas ? "TTAS" : "TAS", n, v);
    CHECK(counter == n * iters, "%s, %d threads: counter == %ld, expected %ld",
          ttas ? "TTAS" : "TAS", n, counter, n * iters);
}

static void test_tas_alone(void)  { check_alone(0); }
static void test_ttas_alone(void) { check_alone(1); }
static void test_tas_four_threads(void)  { check_contended(0, 4); }
static void test_ttas_four_threads(void) { check_contended(1, 4); }

// Benchmark only: printed, never decides PASS/FAIL.
static void bench_worker(int id, void *ctx) {
    (void)id; (void)ctx;
    for (long k = 0; k < iters; k++) {
        lock_it();
        counter++;
        unlock_it();
    }
}

static double timed(int ttas, int n, long total) {
    use_ttas = ttas;
    spin_init(&lk);
    iters = total / n;
    double t0 = bench_now();
    harness_parallel(n, bench_worker, NULL);
    return bench_now() - t0;
}

static void print_benchmark(void) {
    long total = 400000;
    int maxn = harness_ncpus() < 4 ? harness_ncpus() : 4;
    printf("\n  Benchmark (information only, not part of PASS/FAIL): %ld lock/unlock pairs\n", total);
    printf("  | threads | TAS (s) | TTAS (s) |\n  |--------:|--------:|---------:|\n");
    for (int n = 1; n <= maxn; n *= 2)
        printf("  | %7d | %7.4f | %8.4f |\n", n, timed(0, n, total), timed(1, n, total));
    printf("  For a bigger table: make run M=06 EX=ex4_spinlocks\n\n");
}

int main(void) {
    harness_watchdog(50);
    harness_hint("TEST_AND_SET returns the OLD value: false means you just took a free lock. "
                 "unlock() must store false. A timeout means a lock is never released.");
    RUN_TEST(test_tas_alone);
    RUN_TEST(test_ttas_alone);
    RUN_TEST(test_tas_four_threads);
    RUN_TEST(test_ttas_four_threads);
    if (h_tests_failed == 0) {
        printf("  (no violation observed in %ld contended entries per lock)\n", 4 * iters);
        print_benchmark();
    }
    return harness_report();
}
