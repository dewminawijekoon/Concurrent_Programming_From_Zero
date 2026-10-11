// ANSWER - Exercise 5 (from memory): parallel for + reduction
// BUILD: -fopenmp
//
// Test:  make verify-answers M=04
#include <omp.h>

long sum_of_squares(const int *a, long n, int nthreads) {
    long s = 0;
    // Each thread gets a private s that starts at 0; at the end OpenMP adds
    // the private copies into the shared s.
#pragma omp parallel for num_threads(nthreads) reduction(+ : s)
    for (long i = 0; i < n; i++) s += (long)a[i] * a[i];
    return s;
}

int max_value(const int *a, long n, int nthreads) {
    int m = a[0];
    // reduction(max:m): each private copy starts at the smallest int, and
    // the copies are combined by taking the largest.
#pragma omp parallel for num_threads(nthreads) reduction(max : m)
    for (long i = 1; i < n; i++)
        if (a[i] > m) m = a[i];
    return m;
}
