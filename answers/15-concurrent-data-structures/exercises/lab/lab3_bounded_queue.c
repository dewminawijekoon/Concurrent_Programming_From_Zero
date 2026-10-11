// Lab track 3 - a bounded queue with two locks (enqLock and deqLock)
//                                              [Lab 05, Ex 3; L12 slides 17-64]
//
// PREDICT FIRST: two consumers are asleep on an empty queue. Two producers
//                each add one item, and an enqueuer only wakes consumers
//                when the queue goes from EMPTY to non-empty. If it uses
//                pthread_cond_signal (wake ONE), can a consumer sleep
//                forever while an item sits in the queue? (L12 slides 56-61)
//
// The queue is a linked list that always starts with a SENTINEL node (a dummy
// node whose value means nothing). head points to the sentinel; the first
// real item is head->next; tail points to the last node.
//
//   empty:    head,tail -> [S] -> NULL
//   2 items:  head -> [S] -> [a] -> [b] <- tail
//
// Enqueuers only touch the tail end and dequeuers only the head end, so they
// use DIFFERENT locks and can work at the same time. They share one atomic
// counter, `permits` = free places = capacity - number of items.
//
// enq(v):  lock enqLock
//          while permits == 0: wait on notFull (with enqLock)
//          append the new node after tail; tail = new node
//          old = FETCH_ADD(permits, -1); if old == capacity, the queue WAS empty
//          unlock enqLock
//          if it was empty: lock deqLock, BROADCAST notEmpty, unlock deqLock
// deq():   lock deqLock
//          while permits == capacity (empty): wait on notEmpty (with deqLock)
//          v = head->next->value; the first real node becomes the new sentinel
//          old = FETCH_ADD(permits, +1); if old == 0, the queue WAS full
//          unlock deqLock; free the old sentinel
//          if it was full: lock enqLock, BROADCAST notFull, unlock enqLock
//
// Why take the OTHER lock just to broadcast? A sleeper checks the condition
// and goes to sleep while holding its own lock. If you broadcast without
// that lock, your broadcast can fall in the gap between its check and its
// sleep, and it sleeps forever. Note that we never hold both locks at once,
// so the two locks cannot deadlock each other.
//
// Test:  make test M=15 T=lab3_bounded_queue
// Run:   make run  M=15 EX=lab3_bounded_queue
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "atomics.h"
#include "bench.h"

typedef struct node {
    int value;
    struct node *next;
} node_t;

typedef struct {
    node_t *head;               // the sentinel; changed only under deqLock
    node_t *tail;               // the last node; changed only under enqLock
    pthread_mutex_t enqLock;
    pthread_mutex_t deqLock;
    pthread_cond_t notFull;     // waited on with enqLock
    pthread_cond_t notEmpty;    // waited on with deqLock
    shared_int permits;         // free places: capacity - items
    int capacity;
} bounded_queue_t;

// ---------------- given ------------------------------------------------------------
void queue_init(bounded_queue_t *q, int cap) {
    node_t *s = malloc(sizeof *s);
    s->value = -1;
    s->next = NULL;
    q->head = q->tail = s;
    q->capacity = cap;
    STORE(q->permits, cap);
    pthread_mutex_init(&q->enqLock, NULL);
    pthread_mutex_init(&q->deqLock, NULL);
    pthread_cond_init(&q->notFull, NULL);
    pthread_cond_init(&q->notEmpty, NULL);
}

void queue_destroy(bounded_queue_t *q) {   // call only when no thread uses the queue
    node_t *n = q->head;
    while (n) {
        node_t *next = n->next;
        free(n);
        n = next;
    }
    pthread_mutex_destroy(&q->enqLock);
    pthread_mutex_destroy(&q->deqLock);
    pthread_cond_destroy(&q->notFull);
    pthread_cond_destroy(&q->notEmpty);
}

// ---------------- your code -----------------------------------------------------------
void queue_enq(bounded_queue_t *q, int value) {
    node_t *n = malloc(sizeof *n);       // allocate outside the lock: less time holding it
    n->value = value;
    n->next = NULL;
    pthread_mutex_lock(&q->enqLock);
    while (LOAD(q->permits) == 0)        // full: wait until a dequeuer makes room
        pthread_cond_wait(&q->notFull, &q->enqLock);
    q->tail->next = n;                   // link first ...
    q->tail = n;
    int old = FETCH_ADD(q->permits, -1); // ... then publish: dequeuers trust permits
    bool was_empty = (old == q->capacity);
    pthread_mutex_unlock(&q->enqLock);
    if (was_empty) {                     // someone may be asleep waiting for an item
        pthread_mutex_lock(&q->deqLock);
        pthread_cond_broadcast(&q->notEmpty);
        pthread_mutex_unlock(&q->deqLock);
    }
}

int queue_deq(bounded_queue_t *q) {
    pthread_mutex_lock(&q->deqLock);
    while (LOAD(q->permits) == q->capacity)   // empty: wait for an enqueuer
        pthread_cond_wait(&q->notEmpty, &q->deqLock);
    node_t *old_sentinel = q->head;
    node_t *first = old_sentinel->next;       // not NULL: permits says there is an item
    int v = first->value;
    q->head = first;                          // the first real node becomes the sentinel
    int old = FETCH_ADD(q->permits, 1);
    bool was_full = (old == 0);
    pthread_mutex_unlock(&q->deqLock);
    free(old_sentinel);                       // nobody else can reach it any more
    if (was_full) {                           // someone may be asleep waiting for room
        pthread_mutex_lock(&q->enqLock);
        pthread_cond_broadcast(&q->notFull);
        pthread_mutex_unlock(&q->enqLock);
    }
    return v;
}

#ifndef TESTING
// ---------------- benchmark (like the lab's) ----------------------------------------------------
#define ITEMS_PER_PRODUCER 10000
#define POISON (-999)

typedef struct {
    bounded_queue_t *q;
    int id;
    long sum, count;
} targ;

static void *producer(void *v) {
    targ *a = v;
    for (int i = 0; i < ITEMS_PER_PRODUCER; i++) queue_enq(a->q, a->id * 100000 + i);
    return NULL;
}

static void *consumer(void *v) {
    targ *a = v;
    for (;;) {
        int x = queue_deq(a->q);
        if (x == POISON) return NULL;
        a->sum += x;       // each consumer keeps its own totals: no shared lock needed
        a->count++;
    }
}

static void run_test(int np, int nc, int cap) {
    bounded_queue_t q;
    queue_init(&q, cap);
    pthread_t pt[16], ct[16];
    targ pa[16], ca[16];
    double t0 = bench_now();
    for (int i = 0; i < nc; i++) {
        ca[i] = (targ){&q, i, 0, 0};
        pthread_create(&ct[i], NULL, consumer, &ca[i]);
    }
    for (int i = 0; i < np; i++) {
        pa[i] = (targ){&q, i, 0, 0};
        pthread_create(&pt[i], NULL, producer, &pa[i]);
    }
    for (int i = 0; i < np; i++) pthread_join(pt[i], NULL);
    for (int i = 0; i < nc; i++) queue_enq(&q, POISON);   // one poison pill per consumer
    for (int i = 0; i < nc; i++) pthread_join(ct[i], NULL);
    double secs = bench_now() - t0;
    long count = 0;
    for (int i = 0; i < nc; i++) count += ca[i].count;
    long want = (long)np * ITEMS_PER_PRODUCER;
    printf("| %9d | %9d | %8d | %8.4f | %6ld / %-6ld | %3s |\n", np, nc, cap, secs, count, want,
           count == want ? "yes" : "NO");
    queue_destroy(&q);
}

int main(void) {
    printf("| producers | consumers | capacity | time (s) |  items (got/expected) | ok? |\n");
    printf("|----------:|----------:|---------:|---------:|:---------------------:|:---:|\n");
    run_test(1, 1, 10);
    run_test(2, 2, 10);
    run_test(4, 4, 10);
    run_test(4, 4, 100);
    run_test(8, 8, 100);
    return 0;
}
#endif
