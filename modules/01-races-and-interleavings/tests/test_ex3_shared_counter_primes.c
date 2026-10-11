// TIMEOUT: 60
#include "harness.h"
#include "ex3_shared_counter_primes.h"
#include "race_window.h"

#define THREADS 4
#define MAXN 200000
static struct counter ctr;
static atomic_int seen[MAXN];   // how many times each value was handed out
static long per_thread;

static void taker(int id, void *ctx) {
    (void)id; (void)ctx;
    for (long k = 0; k < per_thread; k++) {
        long v = get_and_increment(&ctr);
        if (v >= 0 && v < MAXN) atomic_fetch_add(&seen[v], 1);
    }
}

// Part A: 4 threads take numbers. Every number 0..total-1 must be handed out
// exactly once: no duplicates (lost update) and no gaps.
static void test_every_number_handed_out_once(void) {
    counter_init(&ctr, 0);
    per_thread = 20000;
    long total = THREADS * per_thread;
    for (long i = 0; i < MAXN; i++) atomic_store(&seen[i], 0);
    long calls_before = atomic_load(&race_window_calls);
    harness_parallel(THREADS, taker, NULL);
    long dup = 0, missing = 0, first_dup = -1;
    for (long i = 0; i < total; i++) {
        int s = atomic_load(&seen[i]);
        if (s > 1) { dup++; if (first_dup < 0) first_dup = i; }
        if (s == 0) missing++;
    }
    CHECK(dup == 0, "%ld numbers were handed out more than once (the first was %ld): two threads "
                    "read the same value before either wrote it back", dup, first_dup);
    CHECK(missing == 0, "%ld numbers in 0..%ld were never handed out", missing, total - 1);
    CHECK(atomic_load(&race_window_calls) - calls_before >= total,
          "get_and_increment() no longer calls race_window(). Keep it inside the locked part.");
}

static long serial_primes(long limit, long *divisions) {
    long p = 0;
    *divisions = 0;
    for (long x = 1; x <= limit; x++) p += is_prime_counted(x, divisions);
    return p;
}

// Part B: the dynamic version must find the right primes, and the threads
// together must take each number from 1 to limit exactly once.
static void check_dynamic(long limit, int nthreads) {
    struct prime_stats st[PRIMES_MAX_THREADS];
    for (int t = 0; t < PRIMES_MAX_THREADS; t++) st[t] = (struct prime_stats){-1, -1, -1};
    long want_div, want = serial_primes(limit, &want_div);
    long got = count_primes_dynamic(limit, nthreads, st);
    CHECK(got == want, "count_primes_dynamic(%ld, %d) = %ld, expected %ld", limit, nthreads, got, want);
    long taken = 0, div = 0;
    for (int t = 0; t < nthreads; t++) { taken += st[t].taken; div += st[t].divisions; }
    CHECK(taken == limit, "the threads took %ld numbers in total, expected exactly %ld "
                          "(each number from 1 to %ld once)", taken, limit, limit);
    CHECK(div == want_div, "the threads did %ld divisions in total, expected %ld", div, want_div);
}

static void test_dynamic_primes(void) {
    check_dynamic(1000, 1);      // 168 primes
    check_dynamic(100000, 4);    // 9592 primes
    check_dynamic(50000, 7);
}

int main(void) {
    harness_watchdog(50);
    harness_hint("Part A: lock c->lock, read, race_window(), write, unlock, then return the old "
                 "value. Part B: each thread loops on get_and_increment() until it gets a number "
                 "bigger than limit.");
    RUN_TEST(test_every_number_handed_out_once);
    RUN_TEST(test_dynamic_primes);
    if (h_tests_failed == 0)
        printf("  (no duplicate numbers observed in %d calls - a test cannot prove the counter "
               "correct, but an unprotected version fails almost every run)\n", THREADS * 20000);
    return harness_report();
}
