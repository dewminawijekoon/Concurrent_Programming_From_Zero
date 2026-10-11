// Lab track 5 - Reusable barrier from semaphores + iterative averaging   [Lab 04, Ex 5; L09 slide 28]
//
// A BARRIER is a meeting point for n threads: nobody may go past it until
// all n have arrived. A REUSABLE barrier can be used again and again, e.g.
// once per step of a loop.
//
// PREDICT FIRST: a "simple" barrier uses one counter and one semaphore: the
//                last thread to arrive posts the semaphore n times and sets
//                the counter back to 0. Find an order of steps in which a
//                FAST thread, already at the next barrier call, takes a
//                token meant for a SLOW thread that is still at this one.
//                (Then run:  make run M=07 EX=naive_barrier  to see it happen.)
//
// Part A: barrier_init / barrier_wait / barrier_destroy using ONLY
//         semaphores (the struct is in lab5_reusable_barrier.h). Use two
//         phases, as the README explains:
//           phase 1: count in;  the last to arrive opens turnstile1
//           phase 2: count out; the last to leave opens turnstile2
// Part B: average_parallel(): split the inner elements 1..size-2 into
//         nthreads chunks; each step, each thread computes temp[] for its
//         chunk, then copies its chunk back into arr[]. Where do the
//         barrier_wait() calls go, and why do you need TWO per step?
//
// Test:  make test M=07 T=lab5_reusable_barrier
// Run:   make run  M=07 EX=lab5_reusable_barrier
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lab5_reusable_barrier.h"

int barrier_init(barrier_t *b, int n) {
    if (n < 1) return -1;
    b->n = n;
    b->count = 0;
    // TODO: initialise mutex, turnstile1 and turnstile2 (which start open, which closed?)
    return 0;
}

void barrier_destroy(barrier_t *b) {
    (void)b;   // TODO
}

void barrier_wait(barrier_t *b) {
    (void)b;
    // TODO phase 1: count in; the last to arrive lets all n through turnstile1
    // TODO phase 2: count out; the last to leave lets all n through turnstile2
}

// ---------------------------------------------------------------- the application

// Finished: the lab's serial reference, as a function.
void average_serial(double *arr, int size, int iters) {
    double *temp = malloc(sizeof(double) * (size_t)size);
    for (int it = 0; it < iters; it++) {
        for (int i = 1; i < size - 1; i++) temp[i] = (arr[i - 1] + arr[i] + arr[i + 1]) / 3.0;
        for (int i = 1; i < size - 1; i++) arr[i] = temp[i];
    }
    free(temp);
}

typedef struct {
    double *arr, *temp;
    int size, iters, nthreads, id;
    barrier_t *bar;
} job_t;

static void *averager(void *p) {
    job_t *j = p;
    // my chunk of the inner elements 1 .. size-2
    int inner = j->size - 2;
    int lo = 1 + (int)((long)inner * j->id / j->nthreads);
    int hi = 1 + (int)((long)inner * (j->id + 1) / j->nthreads);   // one past my last element
    (void)lo;
    (void)hi;
    // TODO: for each of j->iters steps:
    //         compute j->temp[i] for i in lo..hi-1 (from j->arr)
    //         ... wait for everyone ...
    //         copy j->temp[i] back into j->arr[i] for my chunk
    //         ... wait for everyone ...
    return NULL;
}

void average_parallel(double *arr, int size, int iters, int nthreads) {
    double *temp = malloc(sizeof(double) * (size_t)size);
    pthread_t *t = malloc(sizeof *t * (size_t)nthreads);
    job_t *jobs = malloc(sizeof *jobs * (size_t)nthreads);
    barrier_t bar;
    barrier_init(&bar, nthreads);
    for (int k = 0; k < nthreads; k++) {
        jobs[k] = (job_t){arr, temp, size, iters, nthreads, k, &bar};
        pthread_create(&t[k], NULL, averager, &jobs[k]);
    }
    for (int k = 0; k < nthreads; k++) pthread_join(t[k], NULL);
    barrier_destroy(&bar);
    free(jobs);
    free(t);
    free(temp);
}

#ifndef TESTING
static void fill(double *a, int size) {
    for (int i = 0; i < size; i++) a[i] = (i == 0 || i == size - 1) ? 100.0 : 0.0;
}
int main(void) {
    // Small run, printed like the lab's serial reference.
    double a[10], b[10];
    for (int it = 1; it <= 5; it++) {
        fill(a, 10);
        fill(b, 10);
        average_serial(a, 10, it);
        average_parallel(b, 10, it, 4);
        printf("Iter %d: ", it - 1);
        for (int i = 0; i < 10; i++) printf("%.1f ", b[i]);
        printf("%s\n", memcmp(a, b, sizeof a) == 0 ? "(same as serial)" : "(DIFFERENT from serial!)");
    }
    // The lab's bigger run: SIZE=100, ITERATIONS=20, 2/4/8 threads.
    enum { SIZE = 100, ITER = 20 };
    double ref[SIZE], par[SIZE];
    fill(ref, SIZE);
    average_serial(ref, SIZE, ITER);
    int counts[] = {2, 4, 8};
    for (int k = 0; k < 3; k++) {
        fill(par, SIZE);
        average_parallel(par, SIZE, ITER, counts[k]);
        double maxdiff = 0;
        for (int i = 0; i < SIZE; i++) {
            double d = par[i] > ref[i] ? par[i] - ref[i] : ref[i] - par[i];
            if (d > maxdiff) maxdiff = d;
        }
        printf("SIZE=%d ITER=%d threads=%d: largest difference from serial = %g\n", SIZE, ITER,
               counts[k], maxdiff);
    }
    return 0;
}
#endif
