// Exercise 2 (core) - matrix-vector multiplication by row blocks      [L04 slides 14-17]
//
// PREDICT FIRST: the threads all write into the same array y. Do you need a
//                mutex? Why or why not?
//
// y = A x. A has m rows and n columns, stored row by row: the element in
// row i, column j is A[i * n + j]. Entry y[i] needs only row i of A and x.
//
// TODO: write matvec_pthreads(). Thread t computes y[i] for the rows
//       first = t*m/nthreads  up to  last = (t+1)*m/nthreads  (last excluded),
//       the same block rule as exercise 1. Give each thread a struct with
//       everything it needs (its id, nthreads, m, n and the three pointers).
//       Start all threads, then join all of them before returning.
//
// The test compares your y with matvec_serial() for many sizes, including
// m not divisible by nthreads and more threads than rows.
//
// Test:  make test M=03 T=ex2_matvec
// Run:   make run  M=03 EX=ex2_matvec        (prints speedup tables)
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include "bench.h"

// Given: the serial version.
void matvec_serial(int m, int n, const double *A, const double *x, double *y) {
    for (int i = 0; i < m; i++) {
        double s = 0.0;
        for (int j = 0; j < n; j++) s += A[(long)i * n + j] * x[j];
        y[i] = s;
    }
}

void matvec_pthreads(int m, int n, const double *A, const double *x, double *y, int nthreads) {
    // TODO (you will want a struct type and a worker function above this one)
    (void)m; (void)n; (void)A; (void)x; (void)y; (void)nthreads;
}

#ifndef TESTING
static void bench_shape(int m, int n) {
    double *A = malloc(sizeof *A * (size_t)m * n), *x = malloc(sizeof *x * n),
           *y = malloc(sizeof *y * m);
    for (long k = 0; k < (long)m * n; k++) A[k] = (double)(k % 7) - 3.0;
    for (int j = 0; j < n; j++) x[j] = 1.0 / (j + 1);
    char title[80];
    snprintf(title, sizeof title, "y = A x with A of size %d x %d", m, n);
    bench_table_header(title);
    double t1 = 0;
    for (int p = 1; p <= 8; p *= 2) {
        int reps = bench_reps(5);
        double s[32];
        for (int r = 0; r < reps && r < 32; r++) {
            double t0 = bench_now();
            matvec_pthreads(m, n, A, x, y, p);
            s[r] = bench_now() - t0;
        }
        double t = bench_median(s, reps < 32 ? reps : 32);
        if (p == 1) t1 = t;
        bench_table_row(p, t, t1);
    }
    free(A); free(x); free(y);
}

int main(void) {
    // Same number of elements (4 million), three different shapes.
    bench_shape(1000000, 4);
    bench_shape(2000, 2000);
    bench_shape(4, 1000000);
    printf("\nLook at 4 x 1000000 with 8 threads: there are only 4 rows, so half of the\n"
           "threads get no rows at all, and 8 threads cannot really beat 4. Row blocks only\n"
           "split the work well when there are many more rows than threads.\n");
    return 0;
}
#endif
