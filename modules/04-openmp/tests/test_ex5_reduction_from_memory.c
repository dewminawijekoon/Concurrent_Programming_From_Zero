// BUILD: -fopenmp
// REQUIRES: openmp
// TIMEOUT: 30
#include "omp_test.h"

long sum_of_squares(const int *a, long n, int nthreads);
int max_value(const int *a, long n, int nthreads);

static void check(long n, int p, long peak_at) {
    int *a = malloc(sizeof *a * (size_t)n);
    long want = 0;
    int want_max = -1000000;
    for (long i = 0; i < n; i++) {
        a[i] = (int)((i * 7919) % 2001) - 1500;        // mostly negative numbers
        if (i == peak_at) a[i] = 4321;
        want += (long)a[i] * a[i];
        if (a[i] > want_max) want_max = a[i];
    }
    long got = sum_of_squares(a, n, p);
    int got_max = max_value(a, n, p);
    CHECK(got == want, "sum_of_squares, n=%ld, %d threads: %ld, expected %ld", n, p, got, want);
    CHECK(got_max == want_max, "max_value, n=%ld, %d threads: %d, expected %d", n, p, got_max, want_max);
    free(a);
}

static void test_small(void) { check(1, 1, -1); check(10, 3, 9); }
static void test_large(void) { check(1000003, 8, 777777); check(1000003, 4, 0); }
static void test_repeated(void) {
    long runs = harness_iters(200);
    for (long r = 0; r < runs; r++) check(3000, 8, r % 3000);
}

int main(void) {
    omp_test_skip_under_tsan();
    harness_watchdog(20);
    harness_hint("#pragma omp parallel for num_threads(nthreads) reduction(+:s)  and  "
                 "reduction(max:m). Start m at a[0], not at 0: all numbers may be negative.");
    RUN_TEST(test_small);
    RUN_TEST(test_large);
    RUN_TEST(test_repeated);
    return harness_report();
}
