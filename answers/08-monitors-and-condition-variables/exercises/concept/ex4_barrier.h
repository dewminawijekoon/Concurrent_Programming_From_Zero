// Interface for ex4_barrier.c
#ifndef EX4_BARRIER_H
#define EX4_BARRIER_H
#include <pthread.h>

typedef struct {
    pthread_mutex_t m;
    pthread_cond_t all_here;   // "everyone has arrived in this round"
    int n;                     // how many threads use the barrier
    int arrived;               // how many have arrived in the current round
    unsigned long generation;  // round number: goes up by 1 each time the barrier opens
} cv_barrier;

void cv_barrier_init(cv_barrier *b, int n);
// Wait until all n threads have called it. Returns 1 in exactly one thread
// per round (the last one to arrive) and 0 in all the others.
int cv_barrier_wait(cv_barrier *b);
void cv_barrier_destroy(cv_barrier *b);
#endif
