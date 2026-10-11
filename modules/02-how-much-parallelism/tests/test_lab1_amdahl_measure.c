// TIMEOUT: 60
#include "harness.h"
#include "lab1_amdahl_measure.h"

static void check_sum(long n, int nthreads) {
    int *a = malloc(sizeof *a * (size_t)(n > 0 ? n : 1));
    long long want = 0;
    for (long i = 0; i < n; i++) { a[i] = (int)((i * 7919) % 2001) - 1000; want += a[i]; }
    long long got = parallel_chunk_sum(a, n, nthreads);
    CHECK(got == want, "parallel_chunk_sum(n=%ld, %d threads) = %lld, expected %lld",
          n, nthreads, got, want);
    free(a);
}

static void test_chunk_sum(void) {
    check_sum(10, 1);
    check_sum(1000003, 3);     // does not divide evenly
    check_sum(1000000, 8);
    check_sum(5, 8);           // more threads than numbers
}

static void test_chunk_sum_large_values(void) {
    long n = 3000000;
    int *a = malloc(sizeof *a * (size_t)n);
    for (long i = 0; i < n; i++) a[i] = 2000000000;   // the total needs long long
    long long got = parallel_chunk_sum(a, n, 4);
    CHECK(got == 6000000000000000LL, "sum of 3,000,000 x 2,000,000,000 = %lld, expected "
                                     "6000000000000000 (use long long for the sums)", got);
    free(a);
}

// Synthetic timings: the answers are known exactly.
static void test_estimate_from_phases(void) {
    CHECK_NEAR(estimate_p_from_phases(2.0, 8.0), 0.8, 1e-12);
    CHECK_NEAR(estimate_p_from_phases(9.0, 1.0), 0.1, 1e-12);
    CHECK_NEAR(estimate_p_from_phases(0.0, 5.0), 1.0, 1e-12);
}

static void test_estimate_from_speedup(void) {
    CHECK_NEAR(estimate_p_from_speedup(2.5, 4), 0.8, 1e-12);
    CHECK_NEAR(estimate_p_from_speedup(1.0, 8), 0.0, 1e-12);     // no speedup: nothing parallel
    CHECK_NEAR(estimate_p_from_speedup(8.0, 8), 1.0, 1e-12);     // perfect speedup: all parallel
    // Round trip: Amdahl with p = 0.9 on 6 processors gives 4.0, which must give back 0.9.
    CHECK_NEAR(estimate_p_from_speedup(amdahl_predict(0.9, 6), 6), 0.9, 1e-9);
    CHECK_NEAR(estimate_p_from_speedup(3.0, 1), -1.0, 1e-12);
}

int main(void) {
    harness_watchdog(50);
    harness_hint("Each thread needs its own lo, hi and sum (an array of structs); read the sums "
                 "only after pthread_join. p from a speedup: (1 - 1/s) / (1 - 1/n).");
    RUN_TEST(test_chunk_sum);
    RUN_TEST(test_chunk_sum_large_values);
    RUN_TEST(test_estimate_from_phases);
    RUN_TEST(test_estimate_from_speedup);
    return harness_report();
}
