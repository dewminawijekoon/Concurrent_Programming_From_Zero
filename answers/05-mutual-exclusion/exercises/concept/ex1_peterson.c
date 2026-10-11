// ANSWER - Exercise 1: Peterson's lock                         [L06 slides 23-26]
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
    int j = 1 - i;
    STORE(flag[i], true);     // 1. "I'm interested"            (LockOne's idea)
    STORE(victim, i);         // 2. "you go first"              (LockTwo's idea)
    // 3. Wait only while BOTH hold: the other wants in AND I'm the victim.
    //    - Alone: flag[j] is false, so I go straight in (no LockTwo deadlock).
    //    - Both: victim is whoever wrote it LAST, so exactly one waits
    //      (no LockOne deadlock).
    while (LOAD(flag[j]) && LOAD(victim) == i)
        cpu_relax();
}

void peterson_unlock(int i) {
    STORE(flag[i], false);    // "no longer interested": releases a waiting j
}

// Why the order in lock() matters (PREDICT FIRST question):
// with victim = i written BEFORE flag[i] = true, this interleaving breaks it:
//   T0: victim = 0         T1: victim = 1      T1: flag[1] = true
//   T1: tests flag[0] -> false, enters CS
//   T0: flag[0] = true;  tests victim == 0 -> false (it is 1), enters CS too.
// tools/interleave finds exactly this schedule.

#ifndef TESTING
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
