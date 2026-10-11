// Exercise 4 (challenge) [EXTRA] - a reusable barrier from a mutex and a condition variable
//                                              [L09 slide 28 (barriers)]
//
// PREDICT FIRST: in this "obvious" barrier, the last thread resets arrived
//                to 0 and broadcasts. The others wait with
//                    while (b->arrived < b->n) pthread_cond_wait(...);
//                What do the woken threads see when they re-check the loop?
//
// A barrier is a meeting point: n threads each call cv_barrier_wait(), and
// nobody leaves until all n have arrived. "Reusable" means the same barrier
// works again for the next round, and the next (threads often loop:
// compute, barrier, compute, barrier, ...).
//
// The trick is the GENERATION counter (the round number):
//   cv_barrier_wait():
//     lock; remember my_gen = generation; arrived++;
//     if I am the last (arrived == n):
//         arrived = 0; generation++; broadcast; unlock; return 1;
//     else:
//         wait WHILE generation == my_gen  (my round has not finished yet);
//         unlock; return 0;
// Waiting on "has the round number changed?" instead of "arrived < n" means
// a woken thread is not confused by the reset, nor by fast threads that
// already arrived for the NEXT round.
//
// Test:  make test M=08 T=ex4_barrier
// Run:   make run  M=08 EX=ex4_barrier        (compares with pthread_barrier_t)
#include <pthread.h>
#include <stdio.h>

#include "ex4_barrier.h"

void cv_barrier_init(cv_barrier *b, int n) {
    pthread_mutex_init(&b->m, NULL);
    pthread_cond_init(&b->all_here, NULL);
    b->n = n;
    b->arrived = 0;
    b->generation = 0;
}

int cv_barrier_wait(cv_barrier *b) {
    // TODO: follow the plan in the header comment (generation counter).
    (void)b;
    return 0;
}

void cv_barrier_destroy(cv_barrier *b) {
    pthread_mutex_destroy(&b->m);
    pthread_cond_destroy(&b->all_here);
}

#ifndef TESTING
#include "bench.h"
#define THREADS 4
#define ROUNDS 100000
static cv_barrier mine;
static pthread_barrier_t theirs;

static void *use_mine(void *arg) {
    (void)arg;
    for (int r = 0; r < ROUNDS; r++) cv_barrier_wait(&mine);
    return NULL;
}
static void *use_theirs(void *arg) {
    (void)arg;
    for (int r = 0; r < ROUNDS; r++) pthread_barrier_wait(&theirs);
    return NULL;
}
static double run(void *(*fn)(void *)) {
    pthread_t t[THREADS];
    double t0 = bench_now();
    for (int i = 0; i < THREADS; i++) pthread_create(&t[i], NULL, fn, NULL);
    for (int i = 0; i < THREADS; i++) pthread_join(t[i], NULL);
    return bench_now() - t0;
}

int main(void) {
    cv_barrier_init(&mine, THREADS);
    pthread_barrier_init(&theirs, NULL, THREADS);
    printf("%d threads, %d rounds each\n", THREADS, ROUNDS);
    printf("  cv_barrier (yours):  %.3f s\n", run(use_mine));
    printf("  pthread_barrier_t:   %.3f s\n", run(use_theirs));
    pthread_barrier_destroy(&theirs);
    cv_barrier_destroy(&mine);
    return 0;
}
#endif
