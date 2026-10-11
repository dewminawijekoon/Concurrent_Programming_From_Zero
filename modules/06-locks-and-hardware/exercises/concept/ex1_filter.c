// Exercise 1 (core) - the Filter lock for n threads            [L06 slides 27-33]
//
// Peterson's lock works for 2 threads. The Filter lock stretches the same
// idea to n threads. There are n-1 "waiting rooms" called levels (1 .. n-1).
// To reach the critical section a thread must climb through every level.
// At each level L, the last thread to arrive is the "victim" of that level,
// and it waits while someone else is at level L or higher.
//
//   level[i]  = the highest level thread i is trying to reach (0 = not trying)
//   victim[L] = the thread that arrived at level L last
//
// PREDICT FIRST: with n = 4 threads all calling lock() at once, how many
//                threads can be past level 1? Past level 2? Past level 3?
//                (Check your answer in section 3 of the README.)
//
// Implement filter_init / filter_lock / filter_unlock below.
//   - use LOAD/STORE from atomics.h for every access to level[] and victim[]
//   - inside every waiting loop, call spin_pause(&spins) (see nlock.h)
//
// Test:  make test M=06 T=ex1_filter
// Run:   make run  M=06 EX=ex1_filter ARGS=4     (number of threads, 2..16)
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include "nlock.h"

static int nthreads;                    // n
static shared_int level[MAX_THREADS];   // level[i] for thread i
static shared_int victim[MAX_THREADS];  // victim[L] for level L (L = 1 .. n-1)

void filter_init(int n) {
    nthreads = n;
    for (int k = 0; k < MAX_THREADS; k++) {
        STORE(level[k], 0);
        STORE(victim[k], 0);
    }
}

// Is some thread other than i at level L or higher?
static bool someone_at_or_above(int i, int L) {
    // TODO: look at level[k] for every k != i
    (void)i; (void)L;
    return false;
}

void filter_lock(int i) {
    unsigned spins = 0;
    // TODO: for L = 1 .. n-1:
    //         announce "I am at level L"; make yourself the victim of level L;
    //         wait while (someone else is at level L or higher) AND (you are
    //         still the victim of level L).
    (void)i; (void)spins; (void)someone_at_or_above;
}

void filter_unlock(int i) {
    // TODO: go back down to level 0
    (void)i;
}

#ifndef TESTING
// Stand-alone demo: n threads add to a plain counter under the lock.
#define ITERS 20000
static long counter;

static void *worker(void *arg) {
    int i = (int)(long)arg;
    for (int k = 0; k < ITERS; k++) {
        filter_lock(i);
        counter++;
        filter_unlock(i);
    }
    return NULL;
}

int main(int argc, char **argv) {
    int n = argc > 1 ? atoi(argv[1]) : 4;
    if (n < 2 || n > MAX_THREADS) n = 4;
    pthread_t t[MAX_THREADS];
    filter_init(n);
    for (long i = 0; i < n; i++) pthread_create(&t[i], NULL, worker, (void *)i);
    for (int i = 0; i < n; i++) pthread_join(t[i], NULL);
    printf("%d threads: counter = %ld (expected %d)\n", n, counter, n * ITERS);
    return 0;
}
#endif
