// ANSWER - Exercise 3 (core): removing a loop-carried dependence with a formula   [EXTRA]
// BUILD: -fopenmp
//
// Test:  make verify-answers M=04
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

// a[i] = 0 + 1 + ... + i = i(i+1)/2. Each a[i] now depends only on i, so
// the iterations are independent and can run in any order, on any thread.
void triangular_parallel(long *a, long n, int nthreads) {
#pragma omp parallel for num_threads(nthreads)
    for (long i = 0; i < n; i++) a[i] = i * (i + 1) / 2;
}

// b[i] = 1 + 3 + 5 + ... + (2i+1) = (i+1)^2: the sum of the first i+1 odd
// numbers is a square. Check: 1, 4, 9, 16, ...
void odd_sums_parallel(long *b, long n, int nthreads) {
#pragma omp parallel for num_threads(nthreads)
    for (long i = 0; i < n; i++) b[i] = (i + 1) * (i + 1);
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
