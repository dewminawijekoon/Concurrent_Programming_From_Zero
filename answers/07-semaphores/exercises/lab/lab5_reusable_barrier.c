// Lab track 5 - Reusable barrier from semaphores + iterative averaging   [Lab 04, Ex 5; L09 slide 28]
//
// ANSWER. A two-phase barrier.
//   Phase 1: everybody counts in. The LAST one to arrive opens gate 1 by
//            posting it n times (one token per thread). Everyone takes one.
//   Phase 2: everybody counts out. The LAST one to leave opens gate 2 the
//            same way.
// Why two phases? A fast thread that runs on to the NEXT barrier call must
// not be able to grab a gate-1 token that was meant for a slow thread of
// THIS round. With two phases, nobody can get back to gate 1 until all n
// threads have passed gate 1 and counted out in phase 2, so all n tokens of
// gate 1 have been taken by then. Gate 2 is protected the same way by gate 1.
//
// Test:  make test M=07 T=lab5_reusable_barrier
// Run:   make run  M=07 EX=lab5_reusable_barrier ANS=1
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
    sem_init(&b->mutex, 0, 1);
    sem_init(&b->turnstile1, 0, 0);   // both gates start closed
    sem_init(&b->turnstile2, 0, 0);
    return 0;
}

void barrier_destroy(barrier_t *b) {
    sem_destroy(&b->mutex);
    sem_destroy(&b->turnstile1);
    sem_destroy(&b->turnstile2);
}

void barrier_wait(barrier_t *b) {
    // Phase 1: arrive.
    sem_wait(&b->mutex);
    b->count++;
    if (b->count == b->n)                       // I am the last to arrive:
        for (int i = 0; i < b->n; i++)          // open gate 1 for all n threads
            sem_post(&b->turnstile1);
    sem_post(&b->mutex);
    sem_wait(&b->turnstile1);                   // wait here until everyone has arrived

    // Phase 2: leave.
    sem_wait(&b->mutex);
    b->count--;
    if (b->count == 0)                          // I am the last to leave phase 1:
        for (int i = 0; i < b->n; i++)          // open gate 2 for all n threads
            sem_post(&b->turnstile2);
    sem_post(&b->mutex);
    sem_wait(&b->turnstile2);                   // wait until everyone is through gate 1
}

// ---------------------------------------------------------------- the application

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
    for (int it = 0; it < j->iters; it++) {
        for (int i = lo; i < hi; i++)
            j->temp[i] = (j->arr[i - 1] + j->arr[i] + j->arr[i + 1]) / 3.0;
        barrier_wait(j->bar);   // everyone has read the OLD values before anyone writes
        for (int i = lo; i < hi; i++) j->arr[i] = j->temp[i];
        barrier_wait(j->bar);   // everyone has written the NEW values before the next step reads
    }
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
