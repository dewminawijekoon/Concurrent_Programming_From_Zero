// Exercise 4 (challenge) - a shared sorted linked list, three locking styles
//                                                                     [L04 slides 27-43]
// PREDICT FIRST: with 8 threads and 99.9% of the calls being Member, which
//                of the three styles will be fastest? With 80% Member,
//                10% Insert, 10% Delete? Write your guess down, then run the
//                benchmark.
//
// The list keeps its values in increasing order, without duplicates. The
// plain_ functions below are correct for ONE thread. Your job is to make the
// list safe for many threads in three different ways (ex4_list.h has the
// interface the test uses):
//
//   TODO 1  one mutex for the whole list           (lecture's Solution 1)
//   TODO 2  one mutex in every node, hand-over-hand (lecture's Solution 2)
//   TODO 3  one read-write lock for the whole list  (lecture's read-write lock)
//
// The list starts with a dummy node `head` (a "sentinel"). Real values start
// at head.next. Because head always exists, insert and delete never need a
// special case for the first node.
//
// Test:  make test M=03 T=ex4_list_locks
// Run:   make run  M=03 EX=ex4_list_locks ARGS="200000"     (benchmark tables)
#include <limits.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include "bench.h"
#include "ex4_list.h"

struct node {
    int value;
    struct node *next;
    pthread_mutex_t lock;          // used only by LIST_PER_NODE
};

// `head` is a dummy first node (a "sentinel"). Real values start at
// head.next. Because head always exists, every real node has a node before
// it, so insert and delete never need a special case for "the first node".
// In the per-node version, head.lock plays the role of the lecture's
// separate mutex for the head pointer.
static struct node head = {INT_MIN, NULL, PTHREAD_MUTEX_INITIALIZER};
static enum list_kind kind;
static pthread_mutex_t list_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_rwlock_t list_rwlock = PTHREAD_RWLOCK_INITIALIZER;

static struct node *new_node(int value, struct node *next) {
    struct node *n = malloc(sizeof *n);
    n->value = value;
    n->next = next;
    pthread_mutex_init(&n->lock, NULL);
    return n;
}

static void free_node(struct node *n) {
    pthread_mutex_destroy(&n->lock);
    free(n);
}

// ---------------------------------------------------------------- given:
// plain versions, correct only when ONE thread uses the list at a time.
static int plain_member(int v) {
    struct node *c = head.next;
    while (c != NULL && c->value < v) c = c->next;
    return c != NULL && c->value == v;
}

static int plain_insert(int v) {
    struct node *pred = &head, *c = head.next;
    while (c != NULL && c->value < v) { pred = c; c = c->next; }
    if (c != NULL && c->value == v) return 0;     // already there
    pred->next = new_node(v, c);                  // link it in between pred and c
    return 1;
}

static int plain_delete(int v) {
    struct node *pred = &head, *c = head.next;
    while (c != NULL && c->value < v) { pred = c; c = c->next; }
    if (c == NULL || c->value != v) return 0;     // not there
    pred->next = c->next;                         // unlink c
    free_node(c);
    return 1;
}

// ---------------------------------------------------------------- Solution 1
// TODO 1: one mutex (list_mutex) around every call. Hint: lock, call the
//         plain_ version, unlock, return its result.
static int one_member(int v) { (void)v; (void)plain_member; (void)list_mutex; return 0; }
static int one_insert(int v) { (void)v; (void)plain_insert; return 0; }
static int one_delete(int v) { (void)v; (void)plain_delete; return 0; }

// ---------------------------------------------------------------- Solution 2
// TODO 2: one mutex per node, hand-over-hand. Rules:
//   - lock head.lock first; to move on, lock the NEXT node before you
//     unlock the current one (so you always hold at least one lock);
//   - stop at the first node whose value is >= v (or at the end of the list);
//   - insert: link the new node between pred and c while holding pred's lock;
//   - delete: unlink c while holding the locks of pred AND c, unlock c,
//     free it with free_node(c), then unlock pred.
// Suggestion: write a helper that walks the list and returns pred and c
// still locked, and use it in all three functions.
static int node_member(int v) { (void)v; return 0; }
static int node_insert(int v) { (void)v; return 0; }
static int node_delete(int v) { (void)v; return 0; }

// ---------------------------------------------------------------- Solution 3
// TODO 3: one read-write lock (list_rwlock). Member only reads the list, so
//         it takes the lock for READING (pthread_rwlock_rdlock). Insert and
//         Delete change it, so they take it for WRITING (pthread_rwlock_wrlock).
//         Both are released with pthread_rwlock_unlock.
static int rw_member(int v) { (void)v; (void)list_rwlock; return 0; }
static int rw_insert(int v) { (void)v; return 0; }
static int rw_delete(int v) { (void)v; return 0; }

// ---------------------------------------------------------------- given: dispatch
void list_init(enum list_kind k) {
    list_free();
    kind = k;
}

int list_member(int v) {
    switch (kind) {
    case LIST_ONE_MUTEX: return one_member(v);
    case LIST_PER_NODE: return node_member(v);
    default: return rw_member(v);
    }
}
int list_insert(int v) {
    switch (kind) {
    case LIST_ONE_MUTEX: return one_insert(v);
    case LIST_PER_NODE: return node_insert(v);
    default: return rw_insert(v);
    }
}
int list_delete(int v) {
    switch (kind) {
    case LIST_ONE_MUTEX: return one_delete(v);
    case LIST_PER_NODE: return node_delete(v);
    default: return rw_delete(v);
    }
}

int list_to_array(int *out, int max) {
    int k = 0;
    for (struct node *c = head.next; c != NULL && k < max; c = c->next) out[k++] = c->value;
    return k;
}

void list_free(void) {
    struct node *c = head.next;
    while (c != NULL) {
        struct node *next = c->next;
        free_node(c);
        c = next;
    }
    head.next = NULL;
}

#ifndef TESTING
// ---------------------------------------------------------------- benchmark
#define KEY_RANGE 2000
#define INITIAL_KEYS 1000

static long total_ops;
static int bench_threads;
static double member_share;    // e.g. 0.999 means 99.9% Member

static void *bench_worker(void *arg) {
    unsigned seed = 12345u + (unsigned)(long)arg;
    long ops = total_ops / bench_threads;
    for (long k = 0; k < ops; k++) {
        int key = rand_r(&seed) % KEY_RANGE;
        double r = (double)rand_r(&seed) / RAND_MAX;
        if (r < member_share) list_member(key);
        else if (r < member_share + (1 - member_share) / 2) list_insert(key);
        else list_delete(key);
    }
    return NULL;
}

static double bench_once(enum list_kind k, int p) {
    list_init(k);
    unsigned seed = 1;
    for (int filled = 0; filled < INITIAL_KEYS;) filled += list_insert(rand_r(&seed) % KEY_RANGE);
    bench_threads = p;
    pthread_t tid[64];
    double t0 = bench_now();
    for (long t = 0; t < p; t++) pthread_create(&tid[t], NULL, bench_worker, (void *)t);
    for (int t = 0; t < p; t++) pthread_join(tid[t], NULL);
    return bench_now() - t0;
}

int main(int argc, char *argv[]) {
    total_ops = argc > 1 ? atol(argv[1]) : 200000;
    const char *names[] = {"one mutex for the list", "one mutex per node", "read-write lock"};
    double mixes[] = {0.999, 0.80};
    printf("%ld operations in total, shared among the threads; %d keys at the start\n", total_ops,
           INITIAL_KEYS);
    for (int m = 0; m < 2; m++) {
        for (int k = 0; k < 3; k++) {
            char title[120];
            snprintf(title, sizeof title, "%s, %.1f%% Member / %.2f%% Insert / %.2f%% Delete",
                     names[k], mixes[m] * 100, (1 - mixes[m]) * 50, (1 - mixes[m]) * 50);
            bench_table_header(title);
            member_share = mixes[m];
            double t1 = 0;
            for (int p = 1; p <= 8; p *= 2) {
                double t = bench_once((enum list_kind)k, p);
                if (p == 1) t1 = t;
                bench_table_row(p, t, t1);
            }
        }
    }
    list_free();
    return 0;
}
#endif
