// BUILD: -fopenmp
// REQUIRES: openmp
// TIMEOUT: 30
#include <math.h>

#include "omp_test.h"

double trap_serial(double a, double b, long n);
double trap_critical(double a, double b, long n, int nthreads, int *threads_used);
double trap_reduction(double a, double b, long n, int nthreads);

static const int counts[] = {1, 2, 3, 4, 8};

// The parallel versions add the same areas in another order, so the last
// digits can differ from the serial answer; 1e-9 allows for that only.
static void test_critical(void) {
    for (unsigned k = 0; k < sizeof counts / sizeof *counts; k++) {
        int p = counts[k], used = -1;
        double want = trap_serial(0, 3, 1000003), got = trap_critical(0, 3, 1000003, p, &used);
        CHECK(fabs(got - want) < 1e-9, "trap_critical with %d threads: %.12f, serial gives %.12f",
              p, got, want);
        CHECK(used >= 1 && used <= p, "trap_critical with %d threads: *threads_used = %d", p, used);
    }
    int used;
    CHECK(fabs(trap_critical(0, 3, 1000, 4, &used) - 9.0) < 1e-4, "integral of x^2 on [0,3] should be about 9");
    CHECK(fabs(trap_critical(1, 2, 7, 8, &used) - trap_serial(1, 2, 7)) < 1e-12,
          "n = 7 trapezoids with 8 threads (some threads get none) is wrong");
}

static void test_reduction(void) {
    for (unsigned k = 0; k < sizeof counts / sizeof *counts; k++) {
        int p = counts[k];
        double want = trap_serial(0, 3, 1000003), got = trap_reduction(0, 3, 1000003, p);
        CHECK(fabs(got - want) < 1e-9, "trap_reduction with %d threads: %.12f, serial gives %.12f",
              p, got, want);
    }
}

// Many short runs: a missing critical/reduction shows up as a wrong sum.
static void test_repeated(void) {
    long runs = harness_iters(300), bad = 0;
    int used;
    double want = trap_serial(0, 3, 4000);
    for (long r = 0; r < runs; r++) {
        if (fabs(trap_critical(0, 3, 4000, 8, &used) - want) > 1e-9) bad++;
        if (fabs(trap_reduction(0, 3, 4000, 8) - want) > 1e-9) bad++;
    }
    CHECK(bad == 0, "%ld of %ld short runs gave a wrong total", bad, 2 * runs);
}

int main(void) {
    omp_test_skip_under_tsan();
    harness_watchdog(25);
    harness_hint("total must be declared BEFORE the parallel region (shared) and the per-thread "
                 "sum INSIDE it (private). Use the team size from omp_get_num_threads(), not the "
                 "number you asked for, when you compute the blocks.");
    RUN_TEST(test_critical);
    RUN_TEST(test_reduction);
    RUN_TEST(test_repeated);
    return harness_report();
}
