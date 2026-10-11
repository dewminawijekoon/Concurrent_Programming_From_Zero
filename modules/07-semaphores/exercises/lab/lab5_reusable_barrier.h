// Interface for lab5_reusable_barrier.c (mirror of Lab 04, Ex 5)
#ifndef LAB5_REUSABLE_BARRIER_H
#define LAB5_REUSABLE_BARRIER_H
#include <semaphore.h>

typedef struct {
    int n;              // how many threads must arrive
    int count;          // how many have arrived so far (protected by mutex)
    sem_t mutex;        // protects count
    sem_t turnstile1;   // gate of phase 1: everyone waits here until all have arrived
    sem_t turnstile2;   // gate of phase 2: everyone waits here until all have left phase 1
    // You may add fields if your design needs them.
} barrier_t;

int barrier_init(barrier_t *b, int n);   // returns 0 on success
void barrier_wait(barrier_t *b);         // returns only when all n threads have called it
void barrier_destroy(barrier_t *b);

// The lab's application: iterative averaging of an array.
// Each step, every inner element becomes the average of itself and its two
// neighbours; arr[0] and arr[size-1] never change.
void average_serial(double *arr, int size, int iters);
void average_parallel(double *arr, int size, int iters, int nthreads);
#endif
