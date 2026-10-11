// TIMEOUT: 60
#include "harness.h"
#include "ex4_pfib.h"
#include "pfib_dag.h"

static struct pfib_dag g;

// L02 slide 21/26: P-FIB(4) has work 17, span 8, parallelism 2.125.
static void test_lecture_values(void) {
    CHECK_EQ(pfib_work(4), 17);
    CHECK_EQ(pfib_span(4), 8);
    CHECK_NEAR(pfib_parallelism(4), 2.125, 1e-12);
    CHECK_EQ(pfib_work(0), 1);
    CHECK_EQ(pfib_span(1), 1);
}

// Build the real DAG and count its strands and its longest path; your
// formulas must agree for every n.
static void test_formulas_match_the_dag(void) {
    for (int n = 0; n <= 11; n++) {
        pfib_make(n, &g);
        long w = g.nv, s = pfib_longest_path(&g);
        if (pfib_work(n) != w || pfib_span(n) != s) {
            CHECK(0, "P-FIB(%d): your work/span = %ld/%ld, but the DAG has %ld strands and a "
                     "longest path of %ld", n, pfib_work(n), pfib_span(n), w, s);
            return;
        }
    }
}

static void test_large_n_is_fast(void) {
    CHECK_EQ(pfib_work(40), 662320561L);   // a loop does this instantly
    CHECK_EQ(pfib_span(40), 80);
    CHECK_NEAR(pfib_parallelism(40), 662320561.0 / 80.0, 1e-3);
}

static long expected_spawns(int n, int cutoff) {
    if (n <= cutoff || n <= 1) return 0;
    return 1 + expected_spawns(n - 1, cutoff) + expected_spawns(n - 2, cutoff);
}

static void check_threads(int n, int cutoff, long fib) {
    atomic_long spawns = 0;
    long got = pfib_threads(n, cutoff, &spawns);
    CHECK(got == fib, "pfib_threads(%d, cutoff %d) = %ld, expected fib(%d) = %ld", n, cutoff, got, n, fib);
    long want = expected_spawns(n, cutoff);
    CHECK(atomic_load(&spawns) == want,
          "pfib_threads(%d, cutoff %d) created %ld threads, expected %ld (one per call with n > cutoff)",
          n, cutoff, (long)atomic_load(&spawns), want);
}

static void test_threads(void) {
    check_threads(10, 20, 55);        // below the cutoff: no threads at all
    check_threads(10, 6, 55);         // a few threads
    check_threads(20, 12, 6765);
    check_threads(25, 18, 75025);
}

int main(void) {
    harness_watchdog(50);
    harness_hint("span(n) = max(2 + span(n-1), 3 + span(n-2)). In pfib_threads, count the spawn "
                 "before pthread_create, and join before adding the two results.");
    RUN_TEST(test_lecture_values);
    RUN_TEST(test_formulas_match_the_dag);
    RUN_TEST(test_large_n_is_fast);
    RUN_TEST(test_threads);
    return harness_report();
}
