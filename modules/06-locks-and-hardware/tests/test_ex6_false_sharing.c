// TIMEOUT: 60
#include "bench.h"
#include "harness.h"

size_t padded_size(void);
size_t padded_gap(void);
size_t padded_offset_in_line(void);
void run_counters(int use_padding, int n, long iters, long *out);

static void test_layout(void) {
    CHECK(padded_gap() >= 64, "neighbouring padded counters are only %zu bytes apart; they must "
          "be at least 64 bytes apart (one cache line) to stop false sharing", padded_gap());
    CHECK(padded_size() % 64 == 0, "sizeof(padded_counter) is %zu; make it a multiple of 64",
          padded_size());
    CHECK(padded_offset_in_line() == 0, "padded[0] starts %zu bytes into a cache line; with "
          "_Alignas(CACHE_LINE) every counter starts at a line boundary", padded_offset_in_line());
}

static void test_counts(void) {
    long iters = harness_iters(200000), out[4];
    for (int pad = 0; pad <= 1; pad++) {
        for (int i = 0; i < 4; i++) out[i] = -1;
        run_counters(pad, 4, iters, out);
        for (int i = 0; i < 4; i++)
            CHECK(out[i] == iters, "%s counters: thread %d's counter is %ld, expected %ld",
                  pad ? "padded" : "packed", i, out[i], iters);
    }
}

int main(void) {
    harness_watchdog(40);
    harness_hint("Write _Alignas(CACHE_LINE) in front of the value member, and make padded_worker "
                 "increment padded[id].value.");
    RUN_TEST(test_layout);
    RUN_TEST(test_counts);
    if (h_tests_failed == 0) {
        long out[4], iters = 5000000;
        int n = harness_ncpus() >= 4 ? 4 : 2;
        double t0 = bench_now();
        run_counters(0, n, iters, out);
        double tp = bench_now() - t0;
        t0 = bench_now();
        run_counters(1, n, iters, out);
        double tq = bench_now() - t0;
        printf("  Timing (information only, not part of PASS/FAIL), %d threads x %ld increments:\n"
               "    packed %.3f s, padded %.3f s  ->  padded is %.1fx faster\n"
               "  Full table: make run M=06 EX=ex6_false_sharing\n", n, iters, tp, tq, tp / tq);
    }
    return harness_report();
}
