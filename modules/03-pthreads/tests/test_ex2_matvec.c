// TIMEOUT: 30
#include <math.h>

#include "harness.h"

void matvec_serial(int m, int n, const double *A, const double *x, double *y);
void matvec_pthreads(int m, int n, const double *A, const double *x, double *y, int nthreads);

static void check_case(int m, int n, int p) {
    double *A = malloc(sizeof *A * (size_t)m * n), *x = malloc(sizeof *x * n);
    double *want = malloc(sizeof *want * m), *got = malloc(sizeof *got * m);
    for (long k = 0; k < (long)m * n; k++) A[k] = (double)((k * 37) % 11) - 5.0;
    for (int j = 0; j < n; j++) x[j] = 0.5 * j - 1.0;
    for (int i = 0; i < m; i++) got[i] = NAN;      // "not computed yet"
    matvec_serial(m, n, A, x, want);
    matvec_pthreads(m, n, A, x, got, p);
    int bad = 0, first_bad = -1;
    for (int i = 0; i < m; i++)
        if (!(got[i] == want[i])) { if (first_bad < 0) first_bad = i; bad++; }
    CHECK(bad == 0, "%d x %d matrix, %d threads: %d of %d entries of y are wrong "
          "(first wrong: y[%d] = %g, expected %g)", m, n, p, bad, m, first_bad,
          first_bad >= 0 ? got[first_bad] : 0.0, first_bad >= 0 ? want[first_bad] : 0.0);
    free(A); free(x); free(want); free(got);
}

static void test_6x6_three_threads(void) { check_case(6, 6, 3); }   // the slide's example
static void test_rows_not_divisible(void) { check_case(10, 5, 3); check_case(101, 17, 4); }
static void test_more_threads_than_rows(void) { check_case(3, 8, 8); }
static void test_one_thread(void) { check_case(50, 50, 1); }
static void test_big(void) { check_case(1000, 1000, 8); check_case(20000, 3, 7); }

int main(void) {
    harness_watchdog(20);
    harness_hint("Each thread needs its own struct (id, nthreads, m, n, A, x, y). Rows of thread "
                 "t: from t*m/nthreads up to (t+1)*m/nthreads. Join every thread before returning.");
    RUN_TEST(test_6x6_three_threads);
    RUN_TEST(test_rows_not_divisible);
    RUN_TEST(test_more_threads_than_rows);
    RUN_TEST(test_one_thread);
    RUN_TEST(test_big);
    return harness_report();
}
