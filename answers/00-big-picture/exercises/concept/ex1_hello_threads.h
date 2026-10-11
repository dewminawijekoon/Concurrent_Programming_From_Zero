// Interface for ex1_hello_threads.c (shared by the exercise and its test)
#ifndef EX1_HELLO_THREADS_H
#define EX1_HELLO_THREADS_H
#include <pthread.h>

#define HELLO_MAX 64

struct hello_slot {
    int index;      // written by thread i: its own number i
    pthread_t who;  // written by thread i: pthread_self()
    long square;    // written by the main thread after joining: what thread i returned
};

// Start n threads (1 <= n <= HELLO_MAX), fill slots[0..n-1], join them all.
// Returns n on success, -1 on error.
int hello_threads(int n, struct hello_slot slots[]);
#endif
