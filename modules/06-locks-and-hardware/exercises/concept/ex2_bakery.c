// Exercise 2 (core) - Lamport's Bakery lock for n threads      [L06 slides 34-40]
//
// Like the ticket machine at a bakery counter: you take a number, then you
// wait until everyone with a smaller number has been served.
//
//   flag[i]  = true while thread i wants to enter (or is inside)
//   label[i] = thread i's ticket number (it is never reset; it only grows)
//
// lock(i) has two parts:
//   doorway: flag[i] = true; label[i] = (largest label you can see) + 1
//   wait:    wait while some other thread k has flag[k] == true AND
//            (label[k], k) comes before (label[i], i)
//
// "(a, x) comes before (b, y)" means: a < b, or a == b and x < y.
// Two threads can end up with the SAME label (they read the labels at the
// same time), so the thread id breaks the tie.
//
// PREDICT FIRST: the tests check first-come-first-served. If thread 1
//                finishes its doorway before thread 2 starts its doorway,
//                who must enter first - even if thread 2 starts waiting
//                earlier? Why can the Filter lock not promise this?
//
// Implement before(), bakery_doorway(), bakery_wait() and bakery_unlock().
//   - use LOAD/STORE from atomics.h for every access to flag[] and label[]
//   - compute the maximum by reading the labels one at a time (this is
//     allowed: the lock stays correct even though max() is not atomic)
//   - in the waiting loop, call spin_pause(&spins) (see nlock.h)
//
// Test:  make test M=06 T=ex2_bakery
// Run:   make run  M=06 EX=ex2_bakery ARGS=4     (number of threads, 2..16)
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include "nlock.h"

static int nthreads;
static shared_bool flag[MAX_THREADS];
static shared_long label[MAX_THREADS];

void bakery_init(int n) {
    nthreads = n;
    for (int k = 0; k < MAX_THREADS; k++) {
        STORE(flag[k], false);
        STORE(label[k], 0);
    }
}

// Does (la, a) come before (lb, b)? Compare labels; if equal, compare ids.
static bool before(long la, int a, long lb, int b) {
    // TODO
    (void)la; (void)a; (void)lb; (void)b;
    return false;
}

void bakery_doorway(int i) {
    // TODO: raise your flag, then take a label one bigger than the largest
    //       label you can see (read label[0..n-1] one at a time)
    (void)i;
}

void bakery_wait(int i) {
    unsigned spins = 0;
    // TODO: wait while some k != i has its flag raised and
    //       (label[k], k) comes before (label[i], i)
    (void)i; (void)spins; (void)before;
}

void bakery_lock(int i) {     // given: the doorway, then the waiting part
    bakery_doorway(i);
    bakery_wait(i);
}

void bakery_unlock(int i) {
    // TODO: "I am no longer interested" (leave the label as it is)
    (void)i;
}

#ifndef TESTING
#define ITERS 20000
static long counter;

static void *worker(void *arg) {
    int i = (int)(long)arg;
    for (int k = 0; k < ITERS; k++) {
        bakery_lock(i);
        counter++;
        bakery_unlock(i);
    }
    return NULL;
}

int main(int argc, char **argv) {
    int n = argc > 1 ? atoi(argv[1]) : 4;
    if (n < 2 || n > MAX_THREADS) n = 4;
    pthread_t t[MAX_THREADS];
    bakery_init(n);
    for (long i = 0; i < n; i++) pthread_create(&t[i], NULL, worker, (void *)i);
    for (int i = 0; i < n; i++) pthread_join(t[i], NULL);
    long biggest = 0;
    for (int i = 0; i < n; i++)
        if (LOAD(label[i]) > biggest) biggest = LOAD(label[i]);
    printf("%d threads: counter = %ld (expected %d)\n", n, counter, n * ITERS);
    printf("largest label handed out: %ld  (labels only grow: a real lock would\n"
           "need a plan for when they overflow)\n", biggest);
    return 0;
}
#endif
