// TIMEOUT: 30
#include "harness.h"
#include "ex2_split_the_work.h"

// Check that the blocks of [lo, hi) cover every number exactly once, in order,
// with sizes that differ by at most 1 and the bigger blocks first.
static int check_split(long lo, long hi, int nthreads) {
    long expect_lo = lo, prev_size = -1;
    for (int t = 0; t < nthreads; t++) {
        long a = -999, b = -999;
        split_range(lo, hi, nthreads, t, &a, &b);
        long size = b - a;
        if (a != expect_lo) {
            CHECK(0, "split [%ld,%ld) into %d: thread %d starts at %ld, expected %ld "
                     "(blocks must follow each other with no gap or overlap)",
                  lo, hi, nthreads, t, a, expect_lo);
            return 0;
        }
        if (size < 0 || (prev_size >= 0 && (size > prev_size || prev_size - size > 1))) {
            CHECK(0, "split [%ld,%ld) into %d: thread %d gets %ld numbers after a block of %ld "
                     "(sizes must differ by at most 1, bigger blocks first)",
                  lo, hi, nthreads, t, size, prev_size);
            return 0;
        }
        prev_size = size;
        expect_lo = b;
    }
    CHECK(expect_lo == hi, "split [%ld,%ld) into %d: the last block ends at %ld, expected %ld",
          lo, hi, nthreads, expect_lo, hi);
    return expect_lo == hi;
}

static void test_split_examples(void) {
    long a, b;
    split_range(0, 10, 3, 0, &a, &b); CHECK(a == 0 && b == 4, "[0,10) / 3, thread 0: got [%ld,%ld), expected [0,4)", a, b);
    split_range(0, 10, 3, 1, &a, &b); CHECK(a == 4 && b == 7, "[0,10) / 3, thread 1: got [%ld,%ld), expected [4,7)", a, b);
    split_range(0, 10, 3, 2, &a, &b); CHECK(a == 7 && b == 10, "[0,10) / 3, thread 2: got [%ld,%ld), expected [7,10)", a, b);
}

static void test_split_many_cases(void) {
    long his[] = {1, 2, 7, 10, 100, 101, 1000003};
    for (unsigned k = 0; k < sizeof his / sizeof *his; k++)
        for (int n = 1; n <= 9; n++)
            if (!check_split(5, 5 + his[k], n)) return;
    check_split(0, 3, 8);   // more threads than numbers: some blocks are empty
}

static long serial_divisions(long limit, long *primes) {
    long d = 0;
    *primes = 0;
    for (long x = 1; x <= limit; x++) *primes += is_prime_counted(x, &d);
    return d;
}

static void check_count(long limit, int nthreads) {
    struct thread_stats st[SPLIT_MAX_THREADS];
    for (int t = 0; t < SPLIT_MAX_THREADS; t++) st[t] = (struct thread_stats){-1, -1};
    long want_primes;
    long want_div = serial_divisions(limit, &want_primes);
    long got = count_primes_static(limit, nthreads, st);
    CHECK(got == want_primes, "count_primes_static(%ld, %d) = %ld, expected %ld",
          limit, nthreads, got, want_primes);
    long sum_p = 0, sum_d = 0;
    for (int t = 0; t < nthreads; t++) { sum_p += st[t].primes; sum_d += st[t].divisions; }
    CHECK(sum_p == want_primes, "the per-thread prime counts add up to %ld, expected %ld",
          sum_p, want_primes);
    CHECK(sum_d == want_div,
          "the per-thread division counts add up to %ld, expected %ld "
          "(each thread must count its own divisions with is_prime_counted)",
          sum_d, want_div);
}

static void test_count_primes(void) {
    check_count(100, 1);       // 25 primes
    check_count(100000, 4);    // 9592 primes
    check_count(100000, 7);
}

int main(void) {
    harness_watchdog(20);
    harness_hint("Work out how many numbers come before thread t's block: t * base, plus one "
                 "for each earlier thread that got an extra number.");
    RUN_TEST(test_split_examples);
    RUN_TEST(test_split_many_cases);
    RUN_TEST(test_count_primes);
    return harness_report();
}
