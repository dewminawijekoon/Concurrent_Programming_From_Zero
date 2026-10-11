// TIMEOUT: 60
#include "atomics.h"
#include "harness.h"

long cas_fetch_increment(shared_long *x);
void cas_max(shared_long *x, long v);

#define NT 4
static shared_long x;
static long per_thread;
static unsigned char *seen;       // seen[v] = how many times v was returned
static long *returned[NT];

static void inc_worker(int id, void *ctx) {
    (void)ctx;
    for (long k = 0; k < per_thread; k++) {
        returned[id][k] = cas_fetch_increment(&x);
        if ((k & 1023) == 0) harness_jitter();
    }
}

static void test_single_thread(void) {
    STORE(x, 10);
    CHECK_EQ(cas_fetch_increment(&x), 10);
    CHECK_EQ(cas_fetch_increment(&x), 11);
    CHECK_EQ(LOAD(x), 12);
    STORE(x, 5);
    cas_max(&x, 3);
    CHECK(LOAD(x) == 5, "cas_max(x = 5, 3) changed x to %ld; a smaller value must leave x alone",
          LOAD(x));
    cas_max(&x, 9);
    CHECK(LOAD(x) == 9, "cas_max(x = 5, 9) left x = %ld, expected 9", LOAD(x));
}

// Like fetch-and-increment: every value 0 .. total-1 is handed out exactly once.
static void test_increment_hands_out_each_value_once(void) {
    per_thread = harness_iters(100000);
    long total = NT * per_thread;
    for (int i = 0; i < NT; i++) returned[i] = malloc(sizeof(long) * (size_t)per_thread);
    seen = calloc((size_t)total, 1);
    STORE(x, 0);
    harness_parallel(NT, inc_worker, NULL);
    CHECK(LOAD(x) == total, "final x == %ld, expected %ld (lost updates)", LOAD(x), total);
    long bad = 0, dup = 0;
    for (int i = 0; i < NT; i++)
        for (long k = 0; k < per_thread; k++) {
            long v = returned[i][k];
            if (v < 0 || v >= total) bad++;
            else if (seen[v]++) dup++;
        }
    CHECK(bad == 0, "%ld returned values were outside 0..%ld", bad, total - 1);
    CHECK(dup == 0, "%ld values were returned to two threads: two increments read the same "
          "old value, so one was lost", dup);
    for (int i = 0; i < NT; i++) free(returned[i]);
    free(seen);
}

static long max_per;
static void max_worker(int id, void *ctx) {
    (void)ctx;
    // thread id offers id, id+NT, id+2*NT, ... in a scrambled order
    for (long k = 0; k < max_per; k++) {
        long j = (k * 7919) % max_per;
        cas_max(&x, j * NT + id);
    }
}

static void test_max_from_many_threads(void) {
    max_per = harness_iters(50000);
    STORE(x, -1);
    harness_parallel(NT, max_worker, NULL);
    long want = -1;                   // the largest value any thread offered
    for (int id = 0; id < NT; id++)
        for (long k = 0; k < max_per; k++)
            if (((k * 7919) % max_per) * NT + id > want) want = ((k * 7919) % max_per) * NT + id;
    CHECK(LOAD(x) == want, "after all threads called cas_max, x == %ld, expected the largest "
          "value offered, %ld", LOAD(x), want);
}

int main(void) {
    harness_watchdog(40);
    harness_hint("Read the old value, compute the new one, and retry the CAS until it succeeds. "
                 "Return the value you read in the attempt that succeeded.");
    RUN_TEST(test_single_thread);
    RUN_TEST(test_increment_hands_out_each_value_once);
    RUN_TEST(test_max_from_many_threads);
    return harness_report();
}
