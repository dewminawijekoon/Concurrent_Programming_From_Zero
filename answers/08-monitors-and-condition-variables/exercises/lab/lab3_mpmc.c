// Lab track 3 - many producers, many consumers, one bounded buffer
//                                              [Lab 02, Ex 3; L09 slides 23-26]
//
// PREDICT FIRST: three consumers are asleep because the buffer is empty. The
//                last producer puts its -1 (its "I'm finished" sentinel), and
//                one consumer takes it. What happens to the other two
//                consumers if nobody wakes them up?
//
// M producers and N consumers share one buffer. Producer p puts the items
// p*1000 + 0, p*1000 + 1, ..., p*1000 + (K-1), then one -1. The threads,
// the item numbering, the printing and the timing are written for you below.
// You write the monitor: mpmc_init(), mpmc_put() and mpmc_get().
//
//   mpmc_put(item)  as in lab 2: wait WHILE full, add the item, wake a consumer.
//   mpmc_get()      returns the next REAL item. It never hands a -1 to a
//                   consumer as an item. Instead:
//                     - if it removes a -1, it adds 1 to sentinels_seen and
//                       keeps looking for a real item;
//                     - once all M sentinels have been seen (and so the buffer
//                       is empty), it returns -1, which means "stop".
//                   A consumer that is asleep on an empty buffer must also
//                   wake up and get -1 when the last sentinel is taken.
//                   Who wakes it? (Hint: broadcast.)
//
// Test:  make test M=08 T=lab3_mpmc
// Run:   make run  M=08 EX=lab3_mpmc                       (configurations A, B, C + timing)
//        make run  M=08 EX=lab3_mpmc ARGS="2 2 5 10 print" (M N K size, print every item)
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bench.h"

#define MAX_SIZE 64

int buffer[MAX_SIZE];
int size;                 // buffer size (the lab uses 10)
int head, tail, count;
int producers;            // M: how many sentinels to expect
int sentinels_seen;       // how many -1s consumers have taken out so far
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t not_full = PTHREAD_COND_INITIALIZER;
pthread_cond_t not_empty = PTHREAD_COND_INITIALIZER;

// Empty the buffer, set its size and the number of producers.
void mpmc_init(int buffer_size, int num_producers) {
    size = buffer_size;
    producers = num_producers;
    head = tail = count = 0;
    sentinels_seen = 0;
}

void mpmc_put(int item) {
    pthread_mutex_lock(&mutex);
    while (count == size)
        pthread_cond_wait(&not_full, &mutex);
    buffer[tail] = item;
    tail = (tail + 1) % size;
    count++;
    pthread_cond_signal(&not_empty);
    pthread_mutex_unlock(&mutex);
}

int mpmc_get(void) {
    pthread_mutex_lock(&mutex);
    for (;;) {
        // Sleep while there is nothing to take AND some producer is still going.
        while (count == 0 && sentinels_seen < producers)
            pthread_cond_wait(&not_empty, &mutex);
        if (count == 0) {                 // every producer has finished: stop
            pthread_mutex_unlock(&mutex);
            return -1;
        }
        int item = buffer[head];
        head = (head + 1) % size;
        count--;
        pthread_cond_signal(&not_full);   // one slot became free
        if (item != -1) {
            pthread_mutex_unlock(&mutex);
            return item;
        }
        sentinels_seen++;
        if (sentinels_seen == producers)  // the last one: wake EVERY sleeping consumer
            pthread_cond_broadcast(&not_empty);
    }
}

// ------------------------------------------------------------------------
// Everything below is given: threads, numbering, counting and timing.

static void ignore_item(int consumer, int item) { (void)consumer; (void)item; }
// The test replaces this to record every consumed item.
void (*mpmc_on_consume)(int consumer, int item) = ignore_item;

static int K_items;
static int verbose;
static atomic_long produced_total, consumed_total;

static void *producer(void *arg) {
    int id = (int)(long)arg;
    for (int s = 0; s < K_items; s++) {
        int item = id * 1000 + s;
        mpmc_put(item);
        atomic_fetch_add(&produced_total, 1);
        if (verbose) printf("Producer %d: put %d\n", id, item);
    }
    mpmc_put(-1);                         // sentinel: "producer id has finished"
    return NULL;
}

static void *consumer(void *arg) {
    int id = (int)(long)arg;
    int item;
    while ((item = mpmc_get()) != -1) {
        atomic_fetch_add(&consumed_total, 1);
        mpmc_on_consume(id, item);
        if (verbose) printf("Consumer %d: got %d (from producer %d)\n", id, item, item / 1000);
    }
    return NULL;
}

struct mpmc_result {
    long produced, consumed;
    double seconds;
};

// Run M producers and N consumers, K items per producer (K <= 1000).
struct mpmc_result mpmc_run(int M, int N, int K, int buffer_size) {
    pthread_t *t = malloc(sizeof *t * (size_t)(M + N));
    mpmc_init(buffer_size, M);
    K_items = K;
    atomic_store(&produced_total, 0);
    atomic_store(&consumed_total, 0);
    double t0 = bench_now();
    for (long i = 0; i < N; i++) pthread_create(&t[M + i], NULL, consumer, (void *)i);
    for (long i = 0; i < M; i++) pthread_create(&t[i], NULL, producer, (void *)i);
    for (int i = 0; i < M + N; i++) pthread_join(t[i], NULL);
    struct mpmc_result r = {atomic_load(&produced_total), atomic_load(&consumed_total),
                            bench_now() - t0};
    free(t);
    return r;
}

#ifndef TESTING
static void summary(const char *name, int M, int N, int K, int bsize) {
    struct mpmc_result r = mpmc_run(M, N, K, bsize);
    printf("| %-9s | %d | %d | %4d | %4d | %8ld | %8ld | %-5s | %8.4f |\n", name, M, N, K, bsize,
           r.produced, r.consumed, r.produced == r.consumed ? "yes" : "NO", r.seconds);
}

int main(int argc, char **argv) {
    if (argc >= 5) {                      // one configuration of your choice
        verbose = argc > 5 && strcmp(argv[5], "print") == 0;
        int M = atoi(argv[1]), N = atoi(argv[2]), K = atoi(argv[3]), b = atoi(argv[4]);
        if (K > 1000 || b < 1 || b > MAX_SIZE) { printf("need K <= 1000 and 1 <= size <= %d\n", MAX_SIZE); return 1; }
        printf("| config    | M | N |    K | size | produced | consumed | match |     time |\n");
        printf("|-----------|---|---|------|------|----------|----------|-------|----------|\n");
        summary("custom", M, N, K, b);
        return 0;
    }
    printf("Lab 02 Ex 3 configurations (K = 20 is too fast to time, so the last rows use K = 1000).\n\n");
    printf("| config    | M | N |    K | size | produced | consumed | match |     time |\n");
    printf("|-----------|---|---|------|------|----------|----------|-------|----------|\n");
    summary("A", 3, 1, 20, 10);
    summary("B", 1, 3, 20, 10);
    summary("C", 3, 3, 20, 10);
    summary("A big", 3, 1, 1000, 10);
    summary("B big", 1, 3, 1000, 10);
    summary("C big", 3, 3, 1000, 10);
    summary("C, size 1", 3, 3, 1000, 1);
    return 0;
}
#endif
