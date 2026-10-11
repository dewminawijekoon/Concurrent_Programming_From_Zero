// BUILD: -fopenmp
// REQUIRES: openmp
// TIMEOUT: 30
#include "omp_test.h"

void prefix_sum_serial(const long *in, long *out, long n);
void prefix_sum_parallel(const long *in, long *out, long n, int nthreads);

static int check_case(long n, int p, int verbose) {
    long *in = malloc(sizeof *in * (size_t)(n + 1)), *want = malloc(sizeof *want * (size_t)(n + 1));
    long *got = malloc(sizeof *got * (size_t)(n + 1));
    for (long i = 0; i < n; i++) in[i] = (i * 2654435761u) % 201 - 100;
    for (long i = 0; i <= n; i++) got[i] = -777777;
    prefix_sum_serial(in, want, n);
    prefix_sum_parallel(in, got, n, p);
    long bad = 0, first = -1;
    for (long i = 0; i < n; i++)
        if (got[i] != want[i]) { if (first < 0) first = i; bad++; }
    if (verbose)
        CHECK(bad == 0, "n=%ld, %d threads: %ld wrong entries (first: out[%ld] = %ld, expected %ld)",
              n, p, bad, first, first >= 0 ? got[first] : 0, first >= 0 ? want[first] : 0);
    CHECK(got[n] == -777777, "n=%ld: wrote past the end of out[]", n);
    free(in); free(want); free(got);
    return bad == 0;
}

static void test_example(void) {
    long in[6] = {3, 1, 4, 1, 5, 2}, out[6] = {0};
    prefix_sum_parallel(in, out, 6, 2);
    long want[6] = {3, 4, 8, 9, 14, 16};
    for (int i = 0; i < 6; i++) CHECK(out[i] == want[i], "3 1 4 1 5 2 with 2 threads: out[%d] = %ld, expected %ld", i, out[i], want[i]);
}
static void test_sizes(void) {
    check_case(20, 4, 1);
    check_case(1001, 3, 1);
    check_case(1000000, 8, 1);
    check_case(1, 1, 1);
}
static void test_more_threads_than_items(void) { check_case(3, 8, 1); check_case(0, 4, 1); }
static void test_repeated(void) {      // a missing barrier shows up only sometimes
    long runs = harness_iters(300), bad = 0;
    for (long r = 0; r < runs; r++) bad += !check_case(5000, 8, 0);
    CHECK(bad == 0, "%ld of %ld runs gave a wrong prefix sum (a missing barrier?)", bad, runs);
}

int main(void) {
    omp_test_skip_under_tsan();
    harness_watchdog(25);
    harness_hint("Pass 1 must finish in EVERY thread before the offsets are added up: that is the "
                 "barrier. Only one thread may add up the offsets (single), and pass 2 must wait "
                 "for it (single ends with a barrier, unless you wrote nowait).");
    RUN_TEST(test_example);
    RUN_TEST(test_sizes);
    RUN_TEST(test_more_threads_than_items);
    RUN_TEST(test_repeated);
    return harness_report();
}
