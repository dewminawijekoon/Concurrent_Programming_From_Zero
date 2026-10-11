// ANSWER - Exercise 1 (warm-up): the trapezoidal rule, critical vs reduction   [L04 slides 45-55]
// BUILD: -fopenmp
//
// Test:  make verify-answers M=04
// Run:   make run M=04 EX=ex1_trapezoid ANS=1 ARGS="10000000 4"
#include <omp.h>
#include <stdio.h>
#include <stdlib.h>

double f(double x) { return x * x; }

double trap_serial(double a, double b, long n) {
    double h = (b - a) / n;
    double s = (f(a) + f(b)) / 2.0;
    for (long i = 1; i < n; i++) s += f(a + i * h);
    return s * h;
}

// Version 1, in the style of the lecture: we split the work by hand, each
// thread adds up its own block of trapezoids in a private variable, and one
// critical section per thread adds it to the shared total.
double trap_critical(double a, double b, long n, int nthreads, int *threads_used) {
    double h = (b - a) / n;
    double total = 0.0;                       // shared: declared before the region
#pragma omp parallel num_threads(nthreads)
    {
        int id = omp_get_thread_num();        // declared inside: private
        int p = omp_get_num_threads();        // the number we REALLY got
        if (id == 0) *threads_used = p;
        long first = (long)id * n / p, last = (long)(id + 1) * n / p;
        double mine = 0.0;
        for (long k = first; k < last; k++) {           // trapezoid k: from x_k to x_k+1
            double x0 = a + k * h, x1 = a + (k + 1) * h;
            mine += (f(x0) + f(x1)) / 2.0 * h;
        }
#pragma omp critical
        total += mine;                        // one thread at a time
    }
    return total;
}

// Version 2: let OpenMP split the loop and combine the partial sums.
double trap_reduction(double a, double b, long n, int nthreads) {
    double h = (b - a) / n;
    double s = (f(a) + f(b)) / 2.0;
    // reduction(+:s): every thread gets a private s starting at 0, and at the
    // end OpenMP adds all the private copies to the original s.
#pragma omp parallel for num_threads(nthreads) reduction(+ : s)
    for (long i = 1; i < n; i++) s += f(a + i * h);
    return s * h;
}

#ifndef TESTING
int main(int argc, char *argv[]) {
    long n = argc > 1 ? atol(argv[1]) : 10000000;
    int p = argc > 2 ? atoi(argv[2]) : 4;
    int got = 0;
    double t0 = omp_get_wtime();
    double s = trap_serial(0, 3, n);
    double t1 = omp_get_wtime();
    double c = trap_critical(0, 3, n, p, &got);
    double t2 = omp_get_wtime();
    double r = trap_reduction(0, 3, n, p);
    double t3 = omp_get_wtime();
    printf("integral of x^2 from 0 to 3 (exact: 9)\n");
    printf("serial:    %.12f  %.4f s\n", s, t1 - t0);
    printf("critical:  %.12f  %.4f s  (asked for %d threads, got %d)\n", c, t2 - t1, p, got);
    printf("reduction: %.12f  %.4f s\n", r, t3 - t2);
    return 0;
}
#endif
