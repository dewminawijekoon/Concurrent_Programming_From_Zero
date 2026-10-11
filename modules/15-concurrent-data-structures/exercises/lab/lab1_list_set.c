// Lab track 1 - a sorted linked-list set: one big lock vs hand-over-hand locking
//                                                  [Lab 05, Ex 1; L12 slides 5-6]
//
// PREDICT FIRST: with 8 threads, which version do you expect to be faster:
//                one lock for the whole list, or one lock per node? Write
//                your guess down, then run the benchmark. (Many people are
//                surprised.)
//
// The set is a linked list sorted by key, with no duplicates. It starts with
// a "head" node whose key is INT_MIN, so every real node has a node before it.
//
//   head(-inf) -> 3 -> 8 -> 15 -> NULL
//
// COARSE-GRAINED: lock l->global_lock around the whole operation, and use the
//   same code as the serial version. Simple, but only one thread at a time
//   can use the list.
//
// FINE-GRAINED (hand-over-hand, or "lock coupling"): every node has its own
//   lock. Walk down the list holding the locks of TWO neighbouring nodes,
//   prev and curr:
//       lock(head); prev = head; curr = head->next;
//       if curr: lock(curr)
//       while curr && curr->key < key:
//           unlock(prev); prev = curr; curr = curr->next; if curr: lock(curr)
//   Always take the next lock BEFORE you let go of the previous one (like
//   climbing hand over hand). Then no other thread can change the link
//   prev -> curr while you are looking at it. When the loop ends, you hold
//   prev (and curr if it is not NULL): insert between them, or unlink curr.
//   Then unlock what you hold.
//
// Return values: insert -> 1 if the key was added, 0 if it was already there;
// delete -> 1 if the key was removed, 0 if it was not there; contains -> 1/0.
//
// Test:  make test M=15 T=lab1_list_set
// Run:   make run  M=15 EX=lab1_list_set
#include <limits.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include "bench.h"

typedef struct node {
    int key;
    struct node *next;
    pthread_mutex_t lock;    // used by the fine-grained version only
} node_t;

typedef struct {
    node_t *head;            // the INT_MIN node; never removed
    pthread_mutex_t global_lock;   // used by the coarse-grained version only
} list_t;

// ---------------- given ----------------------------------------------------------
static node_t *new_node(int key, node_t *next) {
    node_t *n = malloc(sizeof *n);
    n->key = key;
    n->next = next;
    pthread_mutex_init(&n->lock, NULL);
    return n;
}

list_t *list_create(void) {
    list_t *l = malloc(sizeof *l);
    l->head = new_node(INT_MIN, NULL);
    pthread_mutex_init(&l->global_lock, NULL);
    return l;
}

void list_destroy(list_t *l) {
    node_t *n = l->head;
    while (n) {
        node_t *next = n->next;
        pthread_mutex_destroy(&n->lock);
        free(n);
        n = next;
    }
    pthread_mutex_destroy(&l->global_lock);
    free(l);
}

// Serial versions: correct for ONE thread only.
int serial_insert(list_t *l, int key) {
    node_t *prev = l->head, *curr = prev->next;
    while (curr && curr->key < key) { prev = curr; curr = curr->next; }
    if (curr && curr->key == key) return 0;
    prev->next = new_node(key, curr);
    return 1;
}

int serial_contains(list_t *l, int key) {
    node_t *curr = l->head->next;
    while (curr && curr->key < key) curr = curr->next;
    return curr && curr->key == key;
}

int serial_delete(list_t *l, int key) {
    node_t *prev = l->head, *curr = prev->next;
    while (curr && curr->key < key) { prev = curr; curr = curr->next; }
    if (!curr || curr->key != key) return 0;
    prev->next = curr->next;
    pthread_mutex_destroy(&curr->lock);
    free(curr);
    return 1;
}

// ---------------- coarse-grained ------------------------------------------------------
int coarse_insert(list_t *l, int key) {
    // TODO: lock l->global_lock, call the serial version, unlock, return its result
    (void)l; (void)key;
    return 0;
}

int coarse_contains(list_t *l, int key) {
    // TODO: lock l->global_lock, call the serial version, unlock, return its result
    (void)l; (void)key;
    return 0;
}

int coarse_delete(list_t *l, int key) {
    // TODO: lock l->global_lock, call the serial version, unlock, return its result
    (void)l; (void)key;
    return 0;
}

// ---------------- fine-grained (hand-over-hand) --------------------------------------------
// Walk to the first node with key >= `key`. On return, *prev is locked, and
// *curr is locked too unless it is NULL. prev->key < key <= curr->key.
static void locate(list_t *l, int key, node_t **prev, node_t **curr) {
    // TODO (suggested helper): the hand-over-hand walk from the top of the
    //       file. On return *prev is locked, and *curr is locked unless NULL.
    (void)l; (void)key;
    *prev = NULL;
    *curr = NULL;
}

static void unlock_both(node_t *prev, node_t *curr) {
    // TODO: unlock curr (if not NULL), then prev
    (void)prev; (void)curr;
}

int fine_insert(list_t *l, int key) {
    // TODO: locate; if curr is NULL or has a different key, link a
    //       new_node(key, curr) after prev; unlock both; return 1 or 0
    (void)l; (void)key; (void)locate; (void)unlock_both;
    return 0;
}

int fine_contains(list_t *l, int key) {
    // TODO
    (void)l; (void)key;
    return 0;
}

int fine_delete(list_t *l, int key) {
    // TODO: locate; if curr has the key, unlink it (prev->next = curr->next),
    //       unlock and destroy its lock, free it. Unlock prev. Why is it safe to
    //       free curr here? (Who could be waiting for curr's lock?)
    (void)l; (void)key;
    return 0;
}

#ifndef TESTING
// ---------------- benchmark (like the lab's) ------------------------------------------------
#define OPS_PER_THREAD 20000
#define KEY_RANGE 1000

typedef struct {
    list_t *l;
    int (*ins)(list_t *, int), (*con)(list_t *, int), (*del)(list_t *, int);
    unsigned seed;
} bench_arg;

static void *bench_worker(void *v) {
    bench_arg *a = v;
    for (int i = 0; i < OPS_PER_THREAD; i++) {
        int key = rand_r(&a->seed) % KEY_RANGE, op = rand_r(&a->seed) % 100;
        if (op < 50) a->con(a->l, key);        // half of the operations only look
        else if (op < 75) a->ins(a->l, key);
        else a->del(a->l, key);
    }
    return NULL;
}

static double bench(int coarse, int p) {
    list_t *l = list_create();
    unsigned s = 1;
    for (int i = 0; i < KEY_RANGE / 2; i++) serial_insert(l, rand_r(&s) % KEY_RANGE);
    pthread_t th[64];
    bench_arg a[64];
    double t0 = bench_now();
    for (int i = 0; i < p; i++) {
        a[i] = coarse ? (bench_arg){l, coarse_insert, coarse_contains, coarse_delete, 42u + i}
                      : (bench_arg){l, fine_insert, fine_contains, fine_delete, 42u + i};
        pthread_create(&th[i], NULL, bench_worker, &a[i]);
    }
    for (int i = 0; i < p; i++) pthread_join(th[i], NULL);
    double secs = bench_now() - t0;
    list_destroy(l);
    return secs;
}

int main(void) {
    printf("Each thread does %d operations (50%% contains, 25%% insert, 25%% delete) on keys "
           "0..%d\n\n", OPS_PER_THREAD, KEY_RANGE - 1);
    printf("| threads | coarse (s) | coarse ops/s | fine (s) | fine ops/s |\n");
    printf("|--------:|-----------:|-------------:|---------:|-----------:|\n");
    int counts[] = {1, 2, 4, 8};
    for (int k = 0; k < 4; k++) {
        int p = counts[k];
        double tc = bench(1, p), tf = bench(0, p);
        double ops = (double)p * OPS_PER_THREAD;
        printf("| %7d | %10.4f | %12.0f | %8.4f | %10.0f |\n", p, tc, ops / tc, tf, ops / tf);
    }
    return 0;
}
#endif
