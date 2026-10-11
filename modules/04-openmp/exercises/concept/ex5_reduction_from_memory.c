// Exercise 5 (from memory) - parallel for + reduction
// BUILD: -fopenmp
//
// Close the README. From a blank function body, write:
//
//   sum_of_squares(a, n, nthreads)  returns a[0]^2 + a[1]^2 + ... + a[n-1]^2
//                                   (use a long for the sum)
//   max_value(a, n, nthreads)       returns the largest element (n >= 1)
//
// Each one is a single loop with "#pragma omp parallel for",
// num_threads(nthreads) and the right reduction clause. No critical, no
// atomic. In the exam you may be asked to write exactly this kind of loop
// and to explain what the reduction clause does behind the scenes.
//
// Test:  make test M=04 T=ex5_reduction_from_memory
#include <omp.h>

long sum_of_squares(const int *a, long n, int nthreads) {
    (void)a; (void)n; (void)nthreads;
    return 0;
}

int max_value(const int *a, long n, int nthreads) {
    (void)a; (void)n; (void)nthreads;
    return 0;
}
