// Exercise 5 (from memory) - Bounded buffer with three semaphores   [L08 (semaphores) slide 25]
//
// Close the README and the lecture slides. Write the bounded buffer again
// from memory: three semaphores, a circular array, put and get.
// This is a classic exam question ("write the producer and consumer").
//
// PREDICT FIRST: write down the three semaphores and their starting values
//                before you type any code.
//
// Test:  make test M=07 T=ex5_bounded_buffer_from_memory   (same tests as ex4)
#include <semaphore.h>

#include "ex4_bounded_buffer.h"

static int buf[BB_MAX_CAPACITY];
static int cap, head, tail;

int bb_init(int capacity) {
    if (capacity < 1 || capacity > BB_MAX_CAPACITY) return -1;
    cap = capacity;
    head = tail = 0;
    (void)buf;
    return 0;   // TODO
}

void bb_destroy(void) {
    // TODO
}

void bb_put(int item) {
    (void)item;   // TODO
}

int bb_get(void) {
    return -1;   // TODO
}
