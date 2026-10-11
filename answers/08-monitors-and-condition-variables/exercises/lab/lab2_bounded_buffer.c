// Lab track 2 - bounded buffer, one producer and one consumer
//                                              [Lab 02, Ex 2; L09 slide 12]
//
// PREDICT FIRST: with buffer size 1, can the producer ever put two items in a
//                row without the consumer taking one in between? So what does
//                the order of "Produced"/"Consumed" lines look like?
//
// A producer puts the numbers 0, 1, ..., N-1 into a shared buffer, then -1
// (a "sentinel": a special value that means "no more items"). A consumer
// takes items out until it gets -1. The buffer is a circular array:
//   - `tail` is where the next item goes in, `head` is where the next item
//     comes out, `count` is how many items are inside;
//   - after the last slot, both wrap round to slot 0: tail = (tail + 1) % size.
//
// Write put() and get() as a MONITOR: every access to the buffer happens
// while holding `mutex`, and threads that cannot go on sleep on a condition
// variable:
//   - put() waits on `not_full` WHILE the buffer is full,
//     then adds the item and signals `not_empty`;
//   - get() waits on `not_empty` WHILE the buffer is empty,
//     then removes the item and signals `not_full`.
// No busy-waiting. (The lab fixes BUFFER_SIZE = 5 with a #define. Here the
// size is passed to bb_init(), so the test can also try size 1.)
//
// Test:  make test M=08 T=lab2_bounded_buffer
// Run:   make run  M=08 EX=lab2_bounded_buffer ARGS="10 5"    (N items, buffer size)
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include "bench.h"

#define MAX_SIZE 64

int buffer[MAX_SIZE];
int size = 5;                  // the lab's BUFFER_SIZE
int head = 0, tail = 0, count = 0;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t not_full = PTHREAD_COND_INITIALIZER;
pthread_cond_t not_empty = PTHREAD_COND_INITIALIZER;

// Empty the buffer and set its size (1..MAX_SIZE). Call it before starting threads.
void bb_init(int buffer_size) {
    size = buffer_size;
    head = tail = count = 0;
}

void put(int item) {
    pthread_mutex_lock(&mutex);
    while (count == size)                       // full: wait for a free slot
        pthread_cond_wait(&not_full, &mutex);
    buffer[tail] = item;
    tail = (tail + 1) % size;
    count++;
    pthread_cond_signal(&not_empty);            // a consumer may be asleep on "empty"
    pthread_mutex_unlock(&mutex);
}

int get(void) {
    pthread_mutex_lock(&mutex);
    while (count == 0)                          // empty: wait for an item
        pthread_cond_wait(&not_empty, &mutex);
    int item = buffer[head];
    head = (head + 1) % size;
    count--;
    pthread_cond_signal(&not_full);             // a producer may be asleep on "full"
    pthread_mutex_unlock(&mutex);
    return item;
}

#ifndef TESTING
static int N;
static int quiet;

static void *producer(void *arg) {
    (void)arg;
    for (int i = 0; i < N; i++) {
        put(i);
        if (!quiet) printf("Produced: %d\n", i);
    }
    put(-1);                                    // sentinel: "no more items"
    return NULL;
}

static void *consumer(void *arg) {
    int *in_order = arg, expected = 0;
    for (;;) {
        int item = get();
        if (item == -1) break;
        if (!quiet) printf("Consumed: %d\n", item);
        if (item != expected++) *in_order = 0;
    }
    if (expected != N) *in_order = 0;
    return NULL;
}

int main(int argc, char **argv) {
    N = argc > 1 ? atoi(argv[1]) : 10;
    int bsize = argc > 2 ? atoi(argv[2]) : 5;
    if (bsize < 1 || bsize > MAX_SIZE) { printf("buffer size must be 1..%d\n", MAX_SIZE); return 1; }
    quiet = N > 200;                            // big N: just time it
    bb_init(bsize);
    int in_order = 1;
    pthread_t prod, cons;
    double t0 = bench_now();
    pthread_create(&prod, NULL, producer, NULL);
    pthread_create(&cons, NULL, consumer, &in_order);
    pthread_join(prod, NULL);
    pthread_join(cons, NULL);
    printf("All items consumed. In order: %s. Buffer size %d, %d items, %.4f s\n",
           in_order ? "yes" : "NO", bsize, N, bench_now() - t0);
    if (quiet) printf("(Try ARGS=\"1000000 1\" and ARGS=\"1000000 5\" to compare buffer sizes.)\n");
    return 0;
}
#endif
