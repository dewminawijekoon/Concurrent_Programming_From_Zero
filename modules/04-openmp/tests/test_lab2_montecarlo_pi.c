// BUILD: -fopenmp
// REQUIRES: openmp
// TIMEOUT: 60
#include <math.h>

#include "omp_test.h"

double pi_critical(long darts, int nthreads);
double pi_atomic(long darts, int nthreads);
double pi_reduction(long darts, int nthreads);
double pi_local_array(long darts, int nthreads);

// With 2,000,000 darts the estimate is within about 0.001 of pi in 2 out
// of 3 runs (one standard error). 0.01 is ten standard errors: a correct
// version essentially never fails it, but a lost update or a shared seed does.
#define DARTS 2000000L
#define TOL 0.01

static double est[4][3];
static const char *name[] = {"pi_critical", "pi_atomic", "pi_reduction", "pi_local_array"};
static double (*fn[])(long, int) = {pi_critical, pi_atomic, pi_reduction, pi_local_array};
static const int counts[] = {1, 4, 8};

static void run_all(int k) {
    for (int c = 0; c < 3; c++) {
        est[k][c] = fn[k](DARTS, counts[c]);
        CHECK(fabs(est[k][c] - M_PI) < TOL, "%s with %d threads: %.6f is not within %.2f of pi",
              name[k], counts[c], est[k][c], TOL);
    }
}

static void test_a_critical(void) { run_all(0); }
static void test_b_atomic(void) { run_all(1); }
static void test_c_reduction(void) { run_all(2); }
static void test_d_local_array(void) { run_all(3); }

// Same seeds and the same loop split give each thread the same darts in
// every version, so the four answers should agree closely.
static void test_all_four_agree(void) {
    for (int c = 0; c < 3; c++)
        for (int k = 1; k < 4; k++)
            CHECK(fabs(est[k][c] - est[0][c]) < 2 * TOL, "%d threads: %s gives %.6f but pi_critical gives %.6f",
                  counts[c], name[k], est[k][c], est[0][c]);
}

// Lost updates in short, heavily contended runs show up as a low count.
// (20,000 darts: one standard error is about 0.012, so 0.1 is 8 of them.)
static void test_repeated_small(void) {
    long runs = harness_iters(100), bad = 0;
    for (long r = 0; r < runs; r++)
        for (int k = 0; k < 4; k++)
            if (fabs(fn[k](20000, 8) - M_PI) > 0.1) bad++;
    CHECK(bad == 0, "%ld of %ld short runs were far from pi (lost updates?)", bad, 4 * runs);
}

int main(void) {
    omp_test_skip_under_tsan();
    harness_watchdog(50);
    harness_hint("Declare the seed INSIDE the parallel region (one per thread) and use #pragma omp for "
                 "to share the loop. Each version needs its own protection: critical, atomic, "
                 "reduction(+:count), or one counter per thread added up after the region.");
    RUN_TEST(test_a_critical);
    RUN_TEST(test_b_atomic);
    RUN_TEST(test_c_reduction);
    RUN_TEST(test_d_local_array);
    RUN_TEST(test_all_four_agree);
    RUN_TEST(test_repeated_small);
    return harness_report();
}
