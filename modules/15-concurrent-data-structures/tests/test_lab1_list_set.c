// TIMEOUT: 90
#include <limits.h>

#include "harness.h"

typedef struct node {
    int key;
    struct node *next;
    pthread_mutex_t lock;
} node_t;
typedef struct {
    node_t *head;
    pthread_mutex_t global_lock;
} list_t;

list_t *list_create(void);
void list_destroy(list_t *l);
int coarse_insert(list_t *l, int key);
int coarse_contains(list_t *l, int key);
int coarse_delete(list_t *l, int key);
int fine_insert(list_t *l, int key);
int fine_contains(list_t *l, int key);
int fine_delete(list_t *l, int key);

typedef struct {
    const char *name;
    int (*ins)(list_t *, int), (*con)(list_t *, int), (*del)(list_t *, int);
} impl;

static const impl impls[2] = {{"coarse", coarse_insert, coarse_contains, coarse_delete},
                              {"fine", fine_insert, fine_contains, fine_delete}};

// After all threads have stopped: is the list strictly increasing?
static int sorted_ok(const char *name, list_t *l) {
    if (l->head->key != INT_MIN) {
        CHECK(0, "%s: the head node was changed", name);
        return 0;
    }
    for (node_t *n = l->head; n && n->next; n = n->next)
        if (n->next->key <= n->key) {
            CHECK(0, "%s: list is not sorted or has a duplicate: %d is followed by %d", name,
                  n->key, n->next->key);
            return 0;
        }
    return 1;
}

static void single_thread(const impl *m) {
    list_t *l = list_create();
    CHECK(m->con(l, 5) == 0, "%s: contains(5) on an empty set must be 0", m->name);
    CHECK(m->ins(l, 5) == 1, "%s: insert(5) into an empty set must return 1", m->name);
    CHECK(m->ins(l, 5) == 0, "%s: inserting 5 again must return 0 (no duplicates)", m->name);
    CHECK(m->ins(l, 1) == 1 && m->ins(l, 9) == 1 && m->ins(l, 7) == 1, "%s: insert 1, 9, 7",
          m->name);
    CHECK(m->con(l, 7) == 1 && m->con(l, 1) == 1 && m->con(l, 9) == 1,
          "%s: contains must find 1, 7 and 9", m->name);
    CHECK(m->con(l, 6) == 0 && m->con(l, 10) == 0 && m->con(l, -3) == 0,
          "%s: contains must not find 6, 10 or -3", m->name);
    CHECK(m->del(l, 6) == 0, "%s: delete(6) of a missing key must return 0", m->name);
    CHECK(m->del(l, 1) == 1 && m->del(l, 9) == 1, "%s: delete the first and the last key",
          m->name);
    CHECK(m->con(l, 1) == 0 && m->con(l, 9) == 0 && m->con(l, 5) == 1 && m->con(l, 7) == 1,
          "%s: after deleting 1 and 9, only 5 and 7 remain", m->name);
    sorted_ok(m->name, l);
    list_destroy(l);
}

static void test_single_thread_coarse(void) { single_thread(&impls[0]); }
static void test_single_thread_fine(void) { single_thread(&impls[1]); }

// ---- concurrent test -----------------------------------------------------------------
// Each thread owns the keys k with k % T == id and keeps its own model of
// them: since nobody else changes those keys, every answer must match the
// model exactly. All threads also fight over a few SHARED keys; for those we
// count successful inserts and deletes and check the totals at the end.
#define T 4
#define OWN 64          // own keys per thread
#define SHARED 16       // shared keys 100000 .. 100015
#define SHARED_BASE 100000

typedef struct {
    const impl *m;
    list_t *l;
    long ops;
    long ins_ok[T][SHARED], del_ok[T][SHARED];
    char present[T * OWN];        // the model of each thread's own keys
    atomic_int wrong_answers;
} conc_ctx;

static void conc_worker(int id, void *vctx) {
    conc_ctx *c = vctx;
    unsigned s = 1234u + 77u * (unsigned)id;
    for (long i = 0; i < c->ops; i++) {
        s = s * 1103515245u + 12345u;
        unsigned r = s >> 8;
        int op = (int)(r % 3);
        if ((r >> 4) % 4 == 0) {          // a shared key
            int k = (int)((r >> 8) % SHARED);
            if (op == 0) c->ins_ok[id][k] += c->m->ins(c->l, SHARED_BASE + k);
            else if (op == 1) c->del_ok[id][k] += c->m->del(c->l, SHARED_BASE + k);
            else c->m->con(c->l, SHARED_BASE + k);
        } else {                          // one of my own keys
            int j = (int)((r >> 8) % OWN), key = id + T * j;
            int want, got;
            if (op == 0) { want = !c->present[key]; got = c->m->ins(c->l, key); if (got) c->present[key] = 1; }
            else if (op == 1) { want = c->present[key]; got = c->m->del(c->l, key); if (got) c->present[key] = 0; }
            else { want = c->present[key]; got = c->m->con(c->l, key); }
            if (got != want && atomic_fetch_add(&c->wrong_answers, 1) < 3)
                printf("    %s: thread %d: %s(%d) returned %d, but only this thread changes "
                       "that key and it expected %d\n", c->m->name, id,
                       op == 0 ? "insert" : op == 1 ? "delete" : "contains", key, got, want);
        }
        if ((i & 255) == 0) harness_jitter();
    }
}

static void concurrent(const impl *m) {
    conc_ctx *c = calloc(1, sizeof *c);
    c->m = m;
    c->l = list_create();
    c->ops = harness_iters(30000);
    harness_parallel(T, conc_worker, c);
    CHECK(atomic_load(&c->wrong_answers) == 0, "%s: %d answers did not match the model", m->name,
          atomic_load(&c->wrong_answers));
    if (sorted_ok(m->name, c->l)) {
        for (int key = 0; key < T * OWN; key++) {
            int in = 0;
            for (node_t *n = c->l->head->next; n; n = n->next) in |= n->key == key;
            if (in != c->present[key]) {
                CHECK(0, "%s: at the end key %d is %s the list, but its thread expected it %s",
                      m->name, key, in ? "in" : "not in", c->present[key] ? "in" : "not in");
                break;
            }
        }
        for (int k = 0; k < SHARED; k++) {
            long net = 0;
            for (int t = 0; t < T; t++) net += c->ins_ok[t][k] - c->del_ok[t][k];
            int in = 0;
            for (node_t *n = c->l->head->next; n; n = n->next) in |= n->key == SHARED_BASE + k;
            if (net != in) {
                CHECK(0, "%s: shared key %d: successful inserts - successful deletes = %ld, but "
                         "the key is %s the list (must be 1 if in, 0 if not): an update was lost",
                      m->name, SHARED_BASE + k, net, in ? "in" : "not in");
                break;
            }
        }
    }
    if (h_checks_failed == 0)
        printf("    (%s: %ld operations per thread on %d threads, no violation observed - a "
               "stress test cannot prove correctness)\n", m->name, c->ops, T);
    list_destroy(c->l);
    free(c);
}

static void test_concurrent_coarse(void) { concurrent(&impls[0]); }
static void test_concurrent_fine(void) { concurrent(&impls[1]); }

int main(void) {
    harness_watchdog(80);
    harness_hint("Fine-grained: lock the next node BEFORE unlocking the previous one, and keep "
                 "both prev and curr locked while you change prev->next. A timeout usually "
                 "means a lock was not released on some return path.");
    RUN_TEST(test_single_thread_coarse);
    RUN_TEST(test_single_thread_fine);
    RUN_TEST(test_concurrent_coarse);
    RUN_TEST(test_concurrent_fine);
    return harness_report();
}
