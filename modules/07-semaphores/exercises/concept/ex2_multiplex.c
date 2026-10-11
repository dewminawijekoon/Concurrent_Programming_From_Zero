// Exercise 2 (warm-up) - Multiplex: at most N threads inside   [L09 slide 27; Lab 04 intro]
//
// A MULTIPLEX is like a critical section, but it lets up to N threads in at
// the same time, not just one. Example: a car park with N spaces, or a
// server that allows at most N open connections.
//
// PREDICT FIRST: what starting value must the semaphore have? What happens
//                to the (N+1)th thread that calls mplex_enter()?
//
// TODO: implement the five functions with ONE semaphore.
//   mplex_try_enter() must never wait: use sem_trywait(), which takes one
//   from the semaphore if its value is above 0 (returns 0), and otherwise
//   returns -1 straight away.
//
// Test:  make test M=07 T=ex2_multiplex
// Run:   make run  M=07 EX=ex2_multiplex
#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdatomic.h>
#include <stdio.h>
#include <unistd.h>

// TODO: declare your semaphore

void mplex_init(int n) {
    (void)n;   // TODO
}

void mplex_destroy(void) {
    // TODO
}

void mplex_enter(void) {
    // TODO: wait for a free place
}

void mplex_exit(void) {
    // TODO: free your place
}

// Returns 1 if the caller got in, 0 if all N places were taken. Never waits.
int mplex_try_enter(void) {
    return 0;   // TODO
}

#ifndef TESTING
#define N 3
#define THREADS 8
static atomic_int inside;
static void *worker(void *arg) {
    long id = (long)arg;
    mplex_enter();
    int now = atomic_fetch_add(&inside, 1) + 1;
    printf("thread %ld in  (inside now: %d)\n", id, now);
    usleep(100000);
    now = atomic_fetch_sub(&inside, 1) - 1;
    printf("thread %ld out (inside now: %d)\n", id, now);
    mplex_exit();
    return NULL;
}
int main(void) {
    pthread_t t[THREADS];
    mplex_init(N);
    for (long i = 0; i < THREADS; i++) pthread_create(&t[i], NULL, worker, (void *)i);
    for (int i = 0; i < THREADS; i++) pthread_join(t[i], NULL);
    mplex_destroy();
    printf("never more than %d inside? look at the numbers above\n", N);
    return 0;
}
#endif
