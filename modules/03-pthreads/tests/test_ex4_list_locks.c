// TIMEOUT: 45
#include "harness.h"
#include "trace.h"
#include "ex4_list.h"

#define KEYS 64          // few keys = threads keep working on neighbouring nodes
#define MAXT 8

static const char *kind_name[] = {"one mutex", "per-node mutexes", "read-write lock"};

// ---------------------------------------------------------------- one thread
static void single_thread_checks(enum list_kind k) {
    list_init(k);
    const char *nm = kind_name[k];
    CHECK(list_insert(5) == 1, "%s: insert(5) into an empty list should return 1", nm);
    CHECK(list_insert(3) == 1, "%s: insert(3) should return 1", nm);
    CHECK(list_insert(8) == 1, "%s: insert(8) should return 1", nm);
    CHECK(list_insert(5) == 0, "%s: insert(5) again should return 0 (no duplicates)", nm);
    CHECK(list_member(3) == 1 && list_member(8) == 1, "%s: member() misses a value", nm);
    CHECK(list_member(4) == 0, "%s: member(4) should be 0", nm);
    CHECK(list_delete(4) == 0, "%s: delete(4) of a missing value should return 0", nm);
    CHECK(list_delete(3) == 1, "%s: delete(3) should return 1", nm);
    CHECK(list_member(3) == 0, "%s: 3 is still there after delete(3)", nm);
    int a[8], n = list_to_array(a, 8);
    CHECK(n == 2 && a[0] == 5 && a[1] == 8, "%s: list should be [5, 8], has %d values", nm, n);
}

static void test_single_thread_one_mutex(void) { single_thread_checks(LIST_ONE_MUTEX); }
static void test_single_thread_per_node(void) { single_thread_checks(LIST_PER_NODE); }
static void test_single_thread_rwlock(void) { single_thread_checks(LIST_RWLOCK); }

// ---------------------------------------------------------------- many threads
// Every thread "owns" the keys with key % nthreads == id. Only the owner
// inserts or deletes a key, but every thread looks up any key, and all of
// them walk through the same nodes. So each thread can predict the exact
// result of every call on its own keys, and we know exactly which values
// must be in the list at the end.
static int nthreads;
static long ops;
static int model[MAXT][KEYS];      // model[id][key] = 1 if key should be in the list
static atomic_long wrong;

static void worker(int id, void *ctx) {
    (void)ctx;
    unsigned s = 0x2545F491u * (unsigned)(id + 1);
    for (long k = 0; k < ops; k++) {
        s ^= s << 13; s ^= s >> 17; s ^= s << 5;
        int key = (int)(s % KEYS), r = (int)((s >> 8) % 100);
        int own = key - key % nthreads + id;          // a key this thread owns
        if (own >= KEYS) own = id;
        if (r < 40) {
            int res = list_member(key);
            if (key % nthreads == id && res != model[id][key]) {
                atomic_fetch_add(&wrong, 1);
                trace_event(id, "member WRONG", key);
            }
        } else if (r < 70) {
            int res = list_insert(own);
            if (res != !model[id][own]) {
                atomic_fetch_add(&wrong, 1);
                trace_event(id, "insert WRONG", own);
            }
            model[id][own] = 1;
        } else {
            int res = list_delete(own);
            if (res != model[id][own]) {
                atomic_fetch_add(&wrong, 1);
                trace_event(id, "delete WRONG", own);
            }
            model[id][own] = 0;
        }
    }
}

static void concurrent_checks(enum list_kind k, int p) {
    const char *nm = kind_name[k];
    list_init(k);
    trace_reset();
    nthreads = p;
    ops = harness_iters(30000);
    atomic_store(&wrong, 0);
    for (int t = 0; t < MAXT; t++)
        for (int key = 0; key < KEYS; key++) model[t][key] = 0;
    for (int key = 0; key < KEYS; key += 2) {          // start with the even keys
        list_insert(key);
        model[key % p][key] = 1;
    }
    harness_parallel(p, worker, NULL);

    long w = atomic_load(&wrong);
    CHECK(w == 0, "%s, %d threads: %ld calls returned the wrong answer (only the owner "
          "changes a key, so the answer is known exactly)", nm, p, w);
    if (w) trace_dump_tail(10);

    int a[KEYS + 16], n = list_to_array(a, KEYS + 16), sorted = 1, expected = 0, missing = 0;
    for (int i = 1; i < n; i++)
        if (a[i - 1] >= a[i]) sorted = 0;
    CHECK(sorted, "%s, %d threads: the list is not in strictly increasing order", nm, p);
    for (int key = 0; key < KEYS; key++) expected += model[key % p][key];
    for (int key = 0; key < KEYS; key++)
        if (model[key % p][key]) {
            int found = 0;
            for (int i = 0; i < n; i++) found |= a[i] == key;
            missing += !found;
        }
    CHECK(n == expected && missing == 0, "%s, %d threads: the list has %d values, expected %d "
          "(%d expected values are missing)", nm, p, n, expected, missing);
}

static void test_concurrent_one_mutex(void) {
    concurrent_checks(LIST_ONE_MUTEX, 4);
    concurrent_checks(LIST_ONE_MUTEX, 8);
}
static void test_concurrent_per_node(void) {
    concurrent_checks(LIST_PER_NODE, 4);
    concurrent_checks(LIST_PER_NODE, 8);
}
static void test_concurrent_rwlock(void) {
    concurrent_checks(LIST_RWLOCK, 4);
    concurrent_checks(LIST_RWLOCK, 8);
}

// A list bug often shows up as a crash (a thread follows a pointer into a
// node that another thread has already freed). Turn it into a clear FAIL.
static void on_crash(int sig) {
    (void)sig;
    static const char msg[] =
        "    FAIL: the program crashed (segmentation fault) while threads used the list.\n"
        "          Usually a thread read a node that another thread had already freed,\n"
        "          or two threads changed the same next pointer at once: a missing lock.\n";
    ssize_t r = write(STDOUT_FILENO, msg, sizeof msg - 1);
    (void)r;
    _exit(1);
}

int main(void) {
    signal(SIGSEGV, on_crash);
    signal(SIGBUS, on_crash);
    harness_watchdog(30);
    harness_hint("Solutions 1 and 3 just wrap the plain_ functions in lock/unlock. For per-node "
                 "locks: lock the next node before unlocking the current one, and free a deleted "
                 "node only while you still hold the lock of the node before it. A timeout means "
                 "some path returns without unlocking.");
    RUN_TEST(test_single_thread_one_mutex);
    RUN_TEST(test_single_thread_per_node);
    RUN_TEST(test_single_thread_rwlock);
    RUN_TEST(test_concurrent_one_mutex);
    RUN_TEST(test_concurrent_per_node);
    RUN_TEST(test_concurrent_rwlock);
    if (h_tests_failed == 0)
        printf("  (no wrong answer observed in %ld calls per run; a stress test cannot prove the "
               "locking correct, `make tsan M=03` also checks for data races)\n", nthreads * ops);
    list_free();
    return harness_report();
}
