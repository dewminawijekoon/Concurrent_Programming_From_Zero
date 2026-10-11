// Shared tests for parallel_for.h (ex2 and ex4).
// Define TEST_STATIC to 1 to also test the two static versions.
#include "harness.h"
#include "parallel_for.h"

#define MAXN 5000
#define MAXT 16

typedef struct {
    atomic_int hits[MAXN];       // how many times body(i) ran
    atomic_int owner[MAXN];      // rank that ran item i
    atomic_ulong tid[MAXT];      // the pthread that used each rank (0 = not seen)
    atomic_int bad_rank;         // a rank outside 0 .. p-1 was passed
    atomic_int rank_moved;       // one rank was used by two different threads
    int p;
} pf_check;

static pf_check chk;

static void check_body(long i, int rank, void *ctx) {
    pf_check *c = ctx;
    if (i < 0 || i >= MAXN) return;
    if (rank < 0 || rank >= c->p || rank >= MAXT) {
        atomic_store(&c->bad_rank, 1);
        return;
    }
    unsigned long me = (unsigned long)pthread_self(), none = 0;
    if (!atomic_compare_exchange_strong(&c->tid[rank], &none, me) && none != me)
        atomic_store(&c->rank_moved, 1);
    atomic_fetch_add(&c->hits[i], 1);
    atomic_store(&c->owner[i], rank);
    // Uneven work, so the threads drift apart and interleave.
    if (i % 7 == 0) {
        volatile double x = 1.0;
        for (int k = 0; k < 2000; k++) x = x * 1.0000001 + 0.5;
    }
    if (i % 97 == 0) harness_jitter();
}

static void reset(int p) {
    memset(&chk, 0, sizeof chk);
    chk.p = p;
}

// Common checks: every item exactly once, sane ranks, one thread per rank,
// different ranks on different threads.
static int common_checks(const char *what, long n, int p) {
    int bad = 0;
    for (long i = 0; i < n; i++) {
        int h = atomic_load(&chk.hits[i]);
        if (h != 1) {
            CHECK(0, "%s n=%ld p=%d: item %ld ran %d times (must be exactly once)", what, n, p, i,
                  h);
            return 1;
        }
    }
    if (atomic_load(&chk.bad_rank)) {
        CHECK(0, "%s n=%ld p=%d: body got a rank outside 0..%d", what, n, p, p - 1);
        bad = 1;
    }
    if (atomic_load(&chk.rank_moved)) {
        CHECK(0, "%s n=%ld p=%d: one rank was used by two different threads", what, n, p);
        bad = 1;
    }
    for (int a = 0; a < p && a < MAXT; a++)
        for (int b = a + 1; b < p && b < MAXT; b++) {
            unsigned long ta = atomic_load(&chk.tid[a]), tb = atomic_load(&chk.tid[b]);
            if (ta && tb && ta == tb) {
                CHECK(0, "%s n=%ld p=%d: ranks %d and %d ran on the SAME thread - start p "
                         "threads, one per rank", what, n, p, a, b);
                return 1;
            }
        }
    return bad;
}

static const long sizes[] = {0, 1, 5, 64, 1001, MAXN};
static const int threads[] = {1, 2, 3, 4, 8};
#define NSIZES (int)(sizeof sizes / sizeof sizes[0])
#define NTHREADS (int)(sizeof threads / sizeof threads[0])

#if TEST_STATIC
static void test_static_block(void) {
    for (int a = 0; a < NSIZES; a++)
        for (int b = 0; b < NTHREADS; b++) {
            long n = sizes[a];
            int p = threads[b];
            reset(p);
            for_static_block(n, p, check_body, &chk);
            if (common_checks("static block", n, p)) return;
            // item i must belong to the block rule's owner
            long q = n / p, rem = n % p;
            for (long i = 0; i < n; i++) {
                long big = rem * (q + 1);
                int want = i < big ? (int)(i / (q + 1)) : (int)(rem + (i - big) / q);
                if (atomic_load(&chk.owner[i]) != want) {
                    CHECK(0, "static block n=%ld p=%d: item %ld ran on rank %d, expected rank %d",
                          n, p, i, atomic_load(&chk.owner[i]), want);
                    return;
                }
            }
        }
}

static void test_static_cyclic(void) {
    for (int a = 0; a < NSIZES; a++)
        for (int b = 0; b < NTHREADS; b++) {
            long n = sizes[a];
            int p = threads[b];
            reset(p);
            for_static_cyclic(n, p, check_body, &chk);
            if (common_checks("static cyclic", n, p)) return;
            for (long i = 0; i < n; i++)
                if (atomic_load(&chk.owner[i]) != (int)(i % p)) {
                    CHECK(0, "static cyclic n=%ld p=%d: item %ld ran on rank %d, expected %d", n,
                          p, i, atomic_load(&chk.owner[i]), (int)(i % p));
                    return;
                }
        }
}
#endif

static void test_dynamic(void) {
    const long chunks[] = {1, 3, 64, 10000};
    for (int a = 0; a < NSIZES; a++)
        for (int b = 0; b < NTHREADS; b++)
            for (int c = 0; c < 4; c++) {
                long n = sizes[a], chunk = chunks[c];
                int p = threads[b];
                reset(p);
                for_dynamic(n, p, chunk, check_body, &chk);
                if (common_checks("dynamic", n, p)) return;
                // a chunk is handed out whole: all its items ran on one rank
                for (long s = 0; s < n; s += chunk)
                    for (long i = s + 1; i < s + chunk && i < n; i++)
                        if (atomic_load(&chk.owner[i]) != atomic_load(&chk.owner[s])) {
                            CHECK(0, "dynamic n=%ld p=%d chunk=%ld: items %ld and %ld are in the "
                                     "same chunk but ran on different ranks",
                                  n, p, chunk, s, i);
                            return;
                        }
            }
}

static void test_dynamic_zero_chunk(void) {
    reset(2);
    for_dynamic(100, 2, 0, check_body, &chk);   // chunk < 1 must behave like chunk 1
    common_checks("dynamic chunk=0", 100, 2);
}

static int run_parallel_for_tests(void) {
    harness_watchdog(40);
    harness_hint("Every item must run exactly once. For dynamic, take a chunk with ONE "
                 "atomic_fetch_add (a separate load and store lets two threads take the same "
                 "chunk). Start p threads and give each its own rank.");
#if TEST_STATIC
    RUN_TEST(test_static_block);
    RUN_TEST(test_static_cyclic);
#endif
    RUN_TEST(test_dynamic);
    RUN_TEST(test_dynamic_zero_chunk);
    if (h_checks_failed == 0)
        printf("  (no double or missing item observed - a stress test cannot prove there is "
               "no race; try make tsan M=11 too)\n");
    return harness_report();
}
