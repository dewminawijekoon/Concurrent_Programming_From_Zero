// Exercise 1 (warm-up) - hello, threads                 [L01 slides 9-10; Lab 01 Ex 1]
//
// PREDICT FIRST: you start 8 threads and each one prints "hello from i".
//                Will the lines always come out in the order 0, 1, ..., 7?
//                Run it a few times and see.
//
// Write hello_threads(n, slots):
//   1. Start n threads. Give thread i its number i through the `arg`
//      parameter of pthread_create: (void *)(long)i.
//   2. Each thread (in hello_worker) writes into slots[i]:
//        - slots[i].index = i
//        - slots[i].who   = pthread_self()   (its own thread id)
//      and then returns i * i as its result: return (void *)(long)(i * i);
//   3. Start ALL the threads first, and only then join them all.
//      For each thread, store the returned value in slots[i].square.
//   4. Return n if everything worked, or -1 if pthread_create failed.
//
// Test:  make test M=00 T=ex1_hello_threads
// Run:   make run  M=00 EX=ex1_hello_threads ARGS=8
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include "ex1_hello_threads.h"

static struct hello_slot *g_slots;   // the array the threads write into

static void *hello_worker(void *arg) {
    // TODO: get i back from arg, fill in g_slots[i].index and g_slots[i].who,
    //       then return i * i (cast to void *).
    (void)arg;
#ifndef TESTING
    printf("hello from a thread (TODO: print my number)\n");
#endif
    return NULL;
}

int hello_threads(int n, struct hello_slot slots[]) {
    if (n < 1 || n > HELLO_MAX) return -1;
    g_slots = slots;
    // TODO: pthread_t t[HELLO_MAX];
    //       start n threads running hello_worker, passing (void *)(long)i,
    //       then join them all and store each result in slots[i].square.
    (void)hello_worker;
    return n;
}

#ifndef TESTING
int main(int argc, char **argv) {
    int n = argc > 1 ? atoi(argv[1]) : 4;
    struct hello_slot slots[HELLO_MAX];
    if (hello_threads(n, slots) != n) {
        fprintf(stderr, "hello_threads failed (n must be 1..%d)\n", HELLO_MAX);
        return 1;
    }
    for (int i = 0; i < n; i++)
        printf("thread %2d returned %ld\n", slots[i].index, slots[i].square);
    return 0;
}
#endif
