// Exercise 4 (core) - Bounded buffer with three semaphores   [L08 (semaphores) slides 19-25]
//
// ANSWER.
//   spaces   counts free slots    (starts at capacity): producers wait on it
//   elements counts full slots    (starts at 0):        consumers wait on it
//   mutex    lets only one thread at a time touch buf/head/tail (starts at 1)
//
// Order matters: wait for a slot/item FIRST, then take the mutex. If you
// took the mutex first and then waited on a full buffer, you would sleep
// while holding the mutex, and no consumer could ever get in to make room:
// deadlock.
//
// Test:  make test M=07 T=ex4_bounded_buffer
// Run:   make run  M=07 EX=ex4_bounded_buffer ANS=1
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>

#include "ex4_bounded_buffer.h"

static int buf[BB_MAX_CAPACITY];
static int cap, head, tail;    // take from head, add at tail
static sem_t spaces, elements, mutex;

int bb_init(int capacity) {
    if (capacity < 1 || capacity > BB_MAX_CAPACITY) return -1;
    cap = capacity;
    head = tail = 0;
    sem_init(&spaces, 0, (unsigned)capacity);
    sem_init(&elements, 0, 0);
    sem_init(&mutex, 0, 1);
    return 0;
}

void bb_destroy(void) {
    sem_destroy(&spaces);
    sem_destroy(&elements);
    sem_destroy(&mutex);
}

void bb_put(int item) {
    sem_wait(&spaces);         // wait for a free slot
    sem_wait(&mutex);          // only one thread moves tail at a time
    buf[tail] = item;
    tail = (tail + 1) % cap;
    sem_post(&mutex);
    sem_post(&elements);       // one more item for the consumers
}

int bb_get(void) {
    sem_wait(&elements);       // wait for an item
    sem_wait(&mutex);          // only one thread moves head at a time
    int item = buf[head];
    head = (head + 1) % cap;
    sem_post(&mutex);
    sem_post(&spaces);         // one more free slot for the producers
    return item;
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
