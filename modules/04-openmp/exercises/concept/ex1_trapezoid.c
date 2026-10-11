// Exercise 1 (warm-up) - the trapezoidal rule, critical vs reduction   [L04 slides 45-55]
// BUILD: -fopenmp
//
// The area under f between a and b is split into n thin trapezoids of width
// h = (b - a) / n. Trapezoid k goes from x_k = a + k*h to x_k+1, and its
// area is (f(x_k) + f(x_k+1)) / 2 * h. trap_serial() adds them up.
//
// PREDICT FIRST: in trap_critical, if you remove "#pragma omp critical",
//                will the answer be wrong every time? Sometimes? Never?
//
// TODO 1: trap_critical - the lecture's way. Open a parallel region with
//         num_threads(nthreads). In it, each thread:
//           - gets its id with omp_get_thread_num() and the team size p
//             with omp_get_num_threads() (thread 0 stores p in
//             *threads_used: OpenMP may give you fewer threads than you asked for);
//           - adds up trapezoids first .. last-1 (first = id*n/p,
//             last = (id+1)*n/p) in a PRIVATE variable;
//           - adds that to the shared total inside "#pragma omp critical".
// TODO 2: trap_reduction - the same result with "#pragma omp parallel for"
//         and a reduction(+:...) clause on the loop of trap_serial.
//
// Test:  make test M=04 T=ex1_trapezoid
// Run:   make run  M=04 EX=ex1_trapezoid ARGS="10000000 4"
#include <omp.h>
#include <stdio.h>
#include <stdlib.h>

double f(double x) { return x * x; }

// Given: the serial version.
double trap_serial(double a, double b, long n) {
    double h = (b - a) / n;
    double s = (f(a) + f(b)) / 2.0;
    for (long i = 1; i < n; i++) s += f(a + i * h);
    return s * h;
}

double trap_critical(double a, double b, long n, int nthreads, int *threads_used) {
    // TODO 1
    (void)a; (void)b; (void)n; (void)nthreads;
    *threads_used = 0;
    return 0.0;
}

double trap_reduction(double a, double b, long n, int nthreads) {
    // TODO 2
    (void)a; (void)b; (void)n; (void)nthreads;
    return 0.0;
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
