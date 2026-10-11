// Exercise 3 (core) - removing a loop-carried dependence with a formula   [EXTRA]
// BUILD: -fopenmp
//
// In  a[i] = a[i-1] + i  every iteration needs the result of the one before.
// That is a loop-carried dependence (README section 8.4). If you just add
// "#pragma omp parallel for", a thread may read a[i-1] before another thread
// has written it, and the result is wrong (see break_it/loop_dependence.c).
//
// Sometimes the dependence disappears if you find a formula ("closed form")
// for a[i] that uses only i.
//
// PREDICT FIRST: write out a[0..5] for the first loop by hand. Do you
//                recognise the numbers?
//
// TODO 1: triangular_parallel - same result as triangular_serial, but each
//         a[i] computed from i alone, in a "#pragma omp parallel for" loop.
// TODO 2: odd_sums_parallel - same for odd_sums_serial (b[0] = 1,
//         b[i] = b[i-1] + 2i + 1). Hint: write out b[0..4].
//
// Test:  make test M=04 T=ex3_closed_form
// Run:   make run  M=04 EX=ex3_closed_form
#include <omp.h>
#include <stdio.h>

// Given: serial versions with a dependence on the previous element.
void triangular_serial(long *a, long n) {
    if (n > 0) a[0] = 0;
    for (long i = 1; i < n; i++) a[i] = a[i - 1] + i;
}
void odd_sums_serial(long *b, long n) {
    if (n > 0) b[0] = 1;
    for (long i = 1; i < n; i++) b[i] = b[i - 1] + 2 * i + 1;
}

void triangular_parallel(long *a, long n, int nthreads) {
    // TODO 1
    (void)a; (void)n; (void)nthreads;
}

void odd_sums_parallel(long *b, long n, int nthreads) {
    // TODO 2
    (void)b; (void)n; (void)nthreads;
}

#ifndef TESTING
int main(void) {
    long a[10], b[10];
    triangular_parallel(a, 10, 4);
    odd_sums_parallel(b, 10, 4);
    for (int i = 0; i < 10; i++) printf("i=%d  a=%ld  b=%ld\n", i, a[i], b[i]);
    return 0;
}
#endif
