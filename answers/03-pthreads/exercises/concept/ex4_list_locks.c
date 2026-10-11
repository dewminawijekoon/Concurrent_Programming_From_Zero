// ANSWER - Exercise 4 (challenge): a shared sorted linked list, three locking styles
//                                                                     [L04 slides 27-43]
// Test:  make verify-answers M=03
// Run:   make run M=03 EX=ex4_list_locks ANS=1 ARGS="200000"
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
// One mutex around every call: only one thread uses the list at any moment.
static int one_member(int v) {
    pthread_mutex_lock(&list_mutex);
    int r = plain_member(v);
    pthread_mutex_unlock(&list_mutex);
    return r;
}
static int one_insert(int v) {
    pthread_mutex_lock(&list_mutex);
    int r = plain_insert(v);
    pthread_mutex_unlock(&list_mutex);
    return r;
}
static int one_delete(int v) {
    pthread_mutex_lock(&list_mutex);
    int r = plain_delete(v);
    pthread_mutex_unlock(&list_mutex);
    return r;
}

// ---------------------------------------------------------------- Solution 2
// Hand-over-hand: lock the next node BEFORE unlocking the current one, like
// climbing a rope. A thread always holds at least one lock while it walks, so
// no other thread can unlink or free the node it is standing on.
//
// find(v) walks until c is the first node with value >= v (or NULL).
// On return the caller holds pred->lock, and also c->lock if c != NULL.
static void find_locked(int v, struct node **pred_out, struct node **c_out) {
    struct node *pred = &head;
    pthread_mutex_lock(&pred->lock);
    struct node *c = pred->next;
    while (c != NULL) {
        pthread_mutex_lock(&c->lock);         // grab the next node first...
        if (c->value >= v) break;
        pthread_mutex_unlock(&pred->lock);    // ...then let go of the one behind
        pred = c;
        c = c->next;                          // safe: we hold pred's lock
    }
    *pred_out = pred;
    *c_out = c;
}

static void release(struct node *pred, struct node *c) {
    if (c != NULL) pthread_mutex_unlock(&c->lock);
    pthread_mutex_unlock(&pred->lock);
}

static int node_member(int v) {
    struct node *pred, *c;
    find_locked(v, &pred, &c);
    int r = c != NULL && c->value == v;
    release(pred, c);
    return r;
}

static int node_insert(int v) {
    struct node *pred, *c;
    find_locked(v, &pred, &c);
    int r = 0;
    if (c == NULL || c->value != v) {
        pred->next = new_node(v, c);          // we hold pred: nobody else can change pred->next
        r = 1;
    }
    release(pred, c);
    return r;
}

static int node_delete(int v) {
    struct node *pred, *c;
    find_locked(v, &pred, &c);
    if (c == NULL || c->value != v) {
        release(pred, c);
        return 0;
    }
    pred->next = c->next;                     // unlink c while holding pred AND c
    pthread_mutex_unlock(&c->lock);
    // Nobody else can reach c now: to get to c a thread must first lock pred,
    // and we still hold pred. So it is safe to free c.
    free_node(c);
    pthread_mutex_unlock(&pred->lock);
    return 1;
}

// ---------------------------------------------------------------- Solution 3
// Read-write lock: Member only reads, so many Members may run together.
// Insert and Delete change the list, so they need the lock for writing.
static int rw_member(int v) {
    pthread_rwlock_rdlock(&list_rwlock);
    int r = plain_member(v);
    pthread_rwlock_unlock(&list_rwlock);
    return r;
}
static int rw_insert(int v) {
    pthread_rwlock_wrlock(&list_rwlock);
    int r = plain_insert(v);
    pthread_rwlock_unlock(&list_rwlock);
    return r;
}
static int rw_delete(int v) {
    pthread_rwlock_wrlock(&list_rwlock);
    int r = plain_delete(v);
    pthread_rwlock_unlock(&list_rwlock);
    return r;
}

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
