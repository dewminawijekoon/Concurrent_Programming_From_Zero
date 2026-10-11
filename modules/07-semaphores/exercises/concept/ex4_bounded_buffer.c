// Exercise 4 (core) - Bounded buffer with semaphores   [L08 (semaphores) slides 19-25]
//
// A BOUNDED BUFFER is a queue with a fixed number of slots (here: a circular
// array). Producers add items at the tail, consumers take items from the
// head. A producer must wait while the buffer is full; a consumer must wait
// while it is empty.
//
// PREDICT FIRST: the lecture's version (slide 25) uses only two semaphores,
//                `spaces` and `elements`. Why is that enough for ONE producer
//                and ONE consumer, but not for two producers? (Think of two
//                producers both writing buf[tail] before either moves tail.)
//
// Do it in two steps:
//   Step 1: two semaphores, spaces (starts at capacity) and elements (starts
//           at 0), like slide 25. Now test_one_producer_one_consumer passes.
//   Step 2: add a third semaphore, `mutex` (starts at 1), around the lines
//           that touch buf/head/tail. Now the many-producers test passes too.
//           Think hard about WHERE the mutex goes: before or after the wait
//           on spaces / elements?
//
// Test:  make test M=07 T=ex4_bounded_buffer
// Run:   make run  M=07 EX=ex4_bounded_buffer
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>

#include "ex4_bounded_buffer.h"

static int buf[BB_MAX_CAPACITY];
static int cap, head, tail;    // take from head, add at tail
// TODO: declare your semaphores

int bb_init(int capacity) {
    if (capacity < 1 || capacity > BB_MAX_CAPACITY) return -1;
    cap = capacity;
    head = tail = 0;
    // TODO: initialise the semaphores
    (void)buf;   // (remove this line once you use buf)
    return 0;
}

void bb_destroy(void) {
    // TODO
}

void bb_put(int item) {
    // TODO: wait for a free slot, store the item at tail, move tail on
    //       (wrap around with % cap), then announce one more item
    (void)item;
}

int bb_get(void) {
    // TODO: wait for an item, read it at head, move head on, then announce
    //       one more free slot
    return -1;
}

#ifndef TESTING
#define ITEMS 10
static void *producer(void *arg) {
    (void)arg;
    for (int i = 1; i <= ITEMS; i++) {
        bb_put(i);
        printf("put %d\n", i);
    }
    return NULL;
}
static void *consumer(void *arg) {
    (void)arg;
    for (int i = 1; i <= ITEMS; i++) printf("            got %d\n", bb_get());
    return NULL;
}
int main(void) {
    pthread_t p, c;
    bb_init(3);
    pthread_create(&p, NULL, producer, NULL);
    pthread_create(&c, NULL, consumer, NULL);
    pthread_join(p, NULL);
    pthread_join(c, NULL);
    bb_destroy();
    return 0;
}
#endif
