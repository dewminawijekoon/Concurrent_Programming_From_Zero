// ms_queue.h - types and helpers for the Michael-Scott lock-free queue
// (shared by ex2_ms_queue.c and ex3_ms_queue_from_memory.c, and the tests).
//
// The queue is a linked list that starts with a SENTINEL (dummy) node:
//     head -> [S] -> [a] -> [b] -> NULL        tail points to [b], or to the
//                                               node just before it (lagging)
//
// MEMORY: nodes come from a pool that is allocated once in msq_init and freed
// only in msq_destroy, after all threads are finished. We never free a node
// while the queue is in use. Why? A thread may have read a pointer to a node
// a moment ago and be just about to read node->next. If another thread freed
// that node in between, the first thread would read freed memory. Even worse,
// malloc could hand the same address out again for a new node, and a CAS that
// compares addresses would wrongly succeed (the "ABA problem", see the
// README). Real programs solve this with hazard pointers or epochs [EXTRA];
// here we simply never reuse a node.
#ifndef MS_QUEUE_H
#define MS_QUEUE_H

#include <stdbool.h>
#include <stdlib.h>

#include "atomics.h"

typedef struct msq_node {
    long value;                          // written once, before the node is linked in
    _Atomic(struct msq_node *) next;     // NULL for the last node
} msq_node;

typedef struct {
    _Atomic(msq_node *) head;            // the sentinel
    _Atomic(msq_node *) tail;            // the last node, or the one just before it
    msq_node *pool;                      // all nodes ever used by this queue
    shared_long pool_used;
    long pool_cap;
} msq_t;

// Take a fresh node from the pool (lock-free: one fetch-and-add). Aborts if the pool is empty.
static inline msq_node *msq_new_node(msq_t *q, long value) {
    long i = FETCH_ADD(q->pool_used, 1);
    if (i >= q->pool_cap) abort();       // make max_nodes in msq_init bigger
    msq_node *n = &q->pool[i];
    n->value = value;
    STORE(n->next, (msq_node *)NULL);
    return n;
}

// max_nodes = how many enqueues you will ever do + 1 (for the first sentinel).
static inline void msq_init(msq_t *q, long max_nodes) {
    q->pool = malloc(sizeof(msq_node) * (size_t)max_nodes);
    q->pool_cap = max_nodes;
    STORE(q->pool_used, 0);
    msq_node *s = msq_new_node(q, 0);
    STORE(q->head, s);
    STORE(q->tail, s);
}

static inline void msq_destroy(msq_t *q) { free(q->pool); }

void msq_enq(msq_t *q, long value);
bool msq_deq(msq_t *q, long *value);   // false if the queue was empty

#endif
