// ANSWER - Exercise 2 (core): matrix-vector multiplication by row blocks   [L04 slides 14-17]
//
// Test:  make verify-answers M=03
// Run:   make run M=03 EX=ex2_matvec ANS=1
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include "bench.h"

// y = A x, where A has m rows and n columns, stored row by row:
// the element in row i, column j is A[i * n + j].
void matvec_serial(int m, int n, const double *A, const double *x, double *y) {
    for (int i = 0; i < m; i++) {
        double s = 0.0;
        for (int j = 0; j < n; j++) s += A[(long)i * n + j] * x[j];
        y[i] = s;
    }
}

struct mv_arg {
    int id, nthreads, m, n;
    const double *A, *x;
    double *y;
};

static void *mv_worker(void *p) {
    struct mv_arg *a = p;
    int first = (int)((long)a->id * a->m / a->nthreads);
    int last = (int)((long)(a->id + 1) * a->m / a->nthreads);
    // Same loop as the serial code, but only over this thread's rows.
    // No lock is needed: every thread WRITES only its own y[first..last-1],
    // and A and x are only READ. Reading shared data at the same time is safe.
    for (int i = first; i < last; i++) {
        double s = 0.0;               // local sum: keeps the inner loop off y[]
        for (int j = 0; j < a->n; j++) s += a->A[(long)i * a->n + j] * a->x[j];
        a->y[i] = s;
    }
    return NULL;
}

void matvec_pthreads(int m, int n, const double *A, const double *x, double *y, int nthreads) {
    pthread_t *tid = malloc(sizeof *tid * (size_t)nthreads);
    struct mv_arg *args = malloc(sizeof *args * (size_t)nthreads);
    for (int t = 0; t < nthreads; t++) {
        args[t] = (struct mv_arg){t, nthreads, m, n, A, x, y};
        pthread_create(&tid[t], NULL, mv_worker, &args[t]);
    }
    for (int t = 0; t < nthreads; t++) pthread_join(tid[t], NULL);
    free(tid);
    free(args);
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
