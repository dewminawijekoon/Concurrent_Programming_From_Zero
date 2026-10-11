// TIMEOUT: 60
#include "atomics.h"
#include "harness.h"

typedef struct snode {
    long value;
    struct snode *next;
} snode;
typedef struct {
    _Atomic(snode *) top;
} tstack;

long cas_fetch_add(shared_long *x, long d);
void atomic_max(shared_long *x, long v);
void tstack_push(tstack *s, snode *n);

#define T 4
static shared_long counter, maxv;
static long per;

static void add_worker(int id, void *ctx) {
    (void)ctx;
    for (long i = 0; i < per; i++) cas_fetch_add(&counter, id + 1);   // threads add 1, 2, 3, 4
}

static void test_fetch_add(void) {
    STORE(counter, 10);
    CHECK(cas_fetch_add(&counter, 5) == 10, "cas_fetch_add must return the OLD value (10)");
    CHECK_EQ(LOAD(counter), 15);
    STORE(counter, 0);
    per = harness_iters(100000);
    harness_parallel(T, add_worker, NULL);
    CHECK(LOAD(counter) == per * (1 + 2 + 3 + 4), "after the threaded adds: %ld, expected %ld",
          LOAD(counter), per * 10);
}

static long thread_max[T];
static void max_worker(int id, void *ctx) {
    (void)ctx;
    unsigned s = 99u + (unsigned)id;
    long best = -1;
    for (long i = 0; i < per; i++) {
        s = s * 1103515245u + 12345u;
        long v = (long)(s >> 4) % 1000000;
        if (v > best) best = v;
        atomic_max(&maxv, v);
    }
    thread_max[id] = best;
}

static void test_max(void) {
    STORE(maxv, 7);
    atomic_max(&maxv, 3);
    CHECK(LOAD(maxv) == 7, "atomic_max(3) must not lower 7");
    atomic_max(&maxv, 9);
    CHECK_EQ(LOAD(maxv), 9);
    STORE(maxv, -1);
    per = harness_iters(100000);
    harness_parallel(T, max_worker, NULL);
    long want = -1;
    for (int t = 0; t < T; t++) if (thread_max[t] > want) want = thread_max[t];
    CHECK(LOAD(maxv) == want, "after the threaded atomic_max calls: %ld, expected %ld",
          LOAD(maxv), want);
}

#define PUSHES 20000
static snode nodes[T][PUSHES];
static tstack st;

static void push_worker(int id, void *ctx) {
    (void)ctx;
    for (int i = 0; i < PUSHES; i++) {
        nodes[id][i].value = id * PUSHES + i;
        tstack_push(&st, &nodes[id][i]);
        if ((i & 255) == 0) harness_jitter();
    }
}

static void test_push(void) {
    atomic_store(&st.top, NULL);
    harness_parallel(T, push_worker, NULL);
    static char seen[T * PUSHES];
    memset(seen, 0, sizeof seen);
    long count = 0, last[T];
    for (int t = 0; t < T; t++) last[t] = PUSHES;
    for (snode *n = atomic_load(&st.top); n && count <= T * PUSHES; n = n->next, count++) {
        long v = n->value;
        if (v < 0 || v >= T * PUSHES || seen[v]) {
            CHECK(0, "node %ld appears twice in the stack (or is not one of ours)", v);
            return;
        }
        seen[v] = 1;
        int t = (int)(v / PUSHES), i = (int)(v % PUSHES);
        if (i >= last[t]) {
            CHECK(0, "thread %d's nodes are not in last-in first-out order", t);
            return;
        }
        last[t] = i;
    }
    CHECK(count == T * PUSHES, "the stack holds %ld nodes, expected %d (a push was lost)", count,
          T * PUSHES);
}

int main(void) {
    harness_watchdog(50);
    harness_hint("Read, compute, CAS; if the CAS fails, read again and recompute. In push, set "
                 "n->next again before every new CAS attempt.");
    RUN_TEST(test_fetch_add);
    RUN_TEST(test_max);
    RUN_TEST(test_push);
    return harness_report();
}
