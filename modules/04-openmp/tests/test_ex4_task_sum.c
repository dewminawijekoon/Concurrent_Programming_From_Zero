// BUILD: -fopenmp
// REQUIRES: openmp
// TIMEOUT: 30
#include "omp_test.h"

long task_sum(const long *a, long n, long cutoff, int nthreads, long *tasks_created);

// How many tasks the recursion described in the exercise creates.
static long expected_tasks(long n, long cutoff) {
    if (n <= cutoff) return 0;
    return 2 + expected_tasks(n / 2, cutoff) + expected_tasks(n - n / 2, cutoff);
}

static void check(long n, long cutoff, int p) {
    long *a = malloc(sizeof *a * (size_t)(n > 0 ? n : 1)), want = 0, made = -1;
    for (long i = 0; i < n; i++) { a[i] = (i * 31) % 17 - 8; want += a[i]; }
    long got = task_sum(a, n, cutoff, p, &made);
    long e = expected_tasks(n, cutoff);
    CHECK(got == want, "n=%ld, cutoff=%ld, %d threads: sum %ld, expected %ld", n, cutoff, p, got,
          want);
    CHECK(made == e, "n=%ld, cutoff=%ld, %d threads: %ld tasks created, expected %ld%s", n, cutoff,
          p, made, e, (p > 1 && e > 0 && made == e * p) ? " (every thread ran the whole recursion: "
                                                          "missing single?)" : "");
    free(a);
}

static void test_no_tasks_below_cutoff(void) { check(100, 1000, 4); }
static void test_small(void) { check(10, 2, 4); check(7, 1, 3); }
static void test_large(void) { check(1000000, 10000, 4); check(1000001, 777, 8); }
static void test_one_thread(void) { check(50000, 100, 1); }
static void test_repeated(void) {     // a missing taskwait shows up as a wrong sum, sometimes
    long runs = harness_iters(200);
    for (long r = 0; r < runs; r++) check(20000, 50, 8);
}

int main(void) {
    omp_test_skip_under_tsan();
    harness_watchdog(25);
    harness_hint("Pattern: parallel -> single -> sum_rec. In sum_rec: two tasks with "
                 "shared(left) / shared(right), atomic += 2, then taskwait BEFORE you use "
                 "left + right.");
    RUN_TEST(test_no_tasks_below_cutoff);
    RUN_TEST(test_small);
    RUN_TEST(test_large);
    RUN_TEST(test_one_thread);
    RUN_TEST(test_repeated);
    return harness_report();
}
