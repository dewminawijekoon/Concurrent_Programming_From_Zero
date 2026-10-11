// Exercise 5 (from memory) - threads + struct arguments + a mutex-protected sum
//
// Close the slides and your other solutions. From a blank function, write
//
//     long threaded_sum(const long *a, long n, int nthreads);
//
// It returns a[0] + a[1] + ... + a[n-1], computed by nthreads threads:
//   - each thread gets its own struct (array pointer, first, last),
//   - each thread adds its block into a LOCAL variable,
//   - then adds that once to a shared total, protected by a pthread_mutex_t,
//   - the caller joins every thread before returning the total.
// Work for any n >= 0 and any nthreads >= 1 (also n < nthreads).
//
// This is the skeleton that most Pthreads exam answers start from. Practise
// until you can write it in five minutes without looking anything up.
//
// Test:  make test M=03 T=ex5_sum_from_memory
#include <pthread.h>
#include <stdlib.h>

long threaded_sum(const long *a, long n, int nthreads) {
    (void)a; (void)n; (void)nthreads;
    return 0;
}
