// Exercise 1 (core) - Peterson's lock for two threads          [L06 slides 23-26]
//
// PREDICT FIRST: in lock(), what goes wrong if you swap the two lines
//                "flag[i] = true" and "victim = i"?  (Check with
//                tools/interleave: copy programs/peterson.il and swap them.)
//
// Implement peterson_lock / peterson_unlock below.
//   - i is this thread's id (0 or 1); the other thread is j = 1 - i
//   - use LOAD/STORE from atomics.h for every access to flag[] and victim,
//     so the CPU behaves like the lecture model (see break_it/peterson_plain.c)
//   - while you spin, call cpu_relax()
//
// Test:  make test M=05 T=ex1_peterson
// Run:   make run  M=05 EX=ex1_peterson
#include <pthread.h>
#include <stdio.h>

#include "atomics.h"
#include "ex1_peterson.h"

static shared_bool flag[2];   // flag[i] == true: thread i wants to enter
static shared_int victim;     // the thread that lets the other go first

void peterson_init(void) {
    STORE(flag[0], false);
    STORE(flag[1], false);
    STORE(victim, 0);
}

void peterson_lock(int i) {
    // TODO: announce interest, defer to the other thread, then wait while
    //       the other thread is interested AND you are the victim.
    (void)i;
}

void peterson_unlock(int i) {
    // TODO: no longer interested.
    (void)i;
}

#ifndef TESTING
// Stand-alone demo: two threads increment a plain counter under the lock.
#define ITERS 1000000
static long counter;

static void *worker(void *arg) {
    int i = (int)(long)arg;
    for (int k = 0; k < ITERS; k++) {
        peterson_lock(i);
        counter++;
        peterson_unlock(i);
    }
    return NULL;
}

int main(void) {
    pthread_t t[2];
    peterson_init();
    for (long i = 0; i < 2; i++) pthread_create(&t[i], NULL, worker, (void *)i);
    for (int i = 0; i < 2; i++) pthread_join(t[i], NULL);
    printf("counter = %ld (expected %d)\n", counter, 2 * ITERS);
    return 0;
}
#endif
