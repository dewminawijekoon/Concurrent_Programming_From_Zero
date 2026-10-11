// TIMEOUT: 60
#include "harness.h"

long counter_global_mutex(int nthreads, long m);
long counter_local_sum(int nthreads, long m);

// The lab asks for N = 1, 2, 4, 8, 16 and M = 100000.
static void check(const char *name, long (*fn)(int, long)) {
    for (int n = 1; n <= 16; n *= 2) {
        long m = 100000, got = fn(n, m);
        CHECK(got == n * m, "%s with N=%d, M=%ld: counter = %ld, expected %ld", name, n, m, got,
              n * m);
    }
}

static void test_part_a_global_mutex(void) { check("Part A (global mutex)", counter_global_mutex); }
static void test_part_b_local_sum(void) { check("Part B (local sum)", counter_local_sum); }

// Many short runs: a missing lock in Part B shows up here more easily.
static void test_repeated(void) {
    long runs = harness_iters(200), bad = 0;
    for (long r = 0; r < runs; r++) {
        if (counter_global_mutex(8, 500) != 4000) bad++;
        if (counter_local_sum(8, 500) != 4000) bad++;
    }
    CHECK(bad == 0, "%ld of %ld short runs gave the wrong count", bad, 2 * runs);
    if (bad == 0)
        printf("    (no lost update observed in %ld runs; `make tsan M=03` checks for races)\n",
               2 * runs);
}

int main(void) {
    harness_watchdog(50);
    harness_hint("Lock before the update and unlock right after it. A timeout usually means a "
                 "lock() without its unlock(): the next thread waits forever (Lab 01 Q3.3).");
    RUN_TEST(test_part_a_global_mutex);
    RUN_TEST(test_part_b_local_sum);
    RUN_TEST(test_repeated);
    return harness_report();
}
