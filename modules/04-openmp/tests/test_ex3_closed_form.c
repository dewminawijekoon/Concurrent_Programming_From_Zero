// BUILD: -fopenmp
// REQUIRES: openmp
// TIMEOUT: 30
#include "omp_test.h"

void triangular_serial(long *a, long n);
void odd_sums_serial(long *b, long n);
void triangular_parallel(long *a, long n, int nthreads);
void odd_sums_parallel(long *b, long n, int nthreads);

static void check(const char *name, void (*ser)(long *, long), void (*par)(long *, long, int),
                  long n, int p) {
    long *want = malloc(sizeof *want * (size_t)n), *got = malloc(sizeof *got * (size_t)n);
    for (long i = 0; i < n; i++) got[i] = -1;
    ser(want, n);
    par(got, n, p);
    long bad = 0, first = -1;
    for (long i = 0; i < n; i++)
        if (got[i] != want[i]) { if (first < 0) first = i; bad++; }
    CHECK(bad == 0, "%s, n=%ld, %d threads: %ld wrong (first: [%ld] = %ld, expected %ld)", name, n,
          p, bad, first, first >= 0 ? got[first] : 0, first >= 0 ? want[first] : 0);
    free(want); free(got);
}

static void test_triangular(void) {
    check("triangular_parallel", triangular_serial, triangular_parallel, 10, 1);
    check("triangular_parallel", triangular_serial, triangular_parallel, 1000003, 4);
    check("triangular_parallel", triangular_serial, triangular_parallel, 5, 8);
}
static void test_odd_sums(void) {
    check("odd_sums_parallel", odd_sums_serial, odd_sums_parallel, 10, 1);
    check("odd_sums_parallel", odd_sums_serial, odd_sums_parallel, 1000003, 4);
    check("odd_sums_parallel", odd_sums_serial, odd_sums_parallel, 5, 8);
}

int main(void) {
    omp_test_skip_under_tsan();
    harness_watchdog(20);
    harness_hint("Write out the first few values of the serial loop and look for a pattern that "
                 "uses only i. Your parallel loop must not read a[i-1].");
    RUN_TEST(test_triangular);
    RUN_TEST(test_odd_sums);
    return harness_report();
}
