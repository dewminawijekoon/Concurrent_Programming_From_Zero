// TIMEOUT: 90
#include "capstone_checks.h"

enum { MODE_BLOCK, MODE_CYCLIC, MODE_DYNAMIC };
long twins_pthreads(long N, int T, int mode, int chunk, long *work);

static const long Ns[] = {5, 100, 1000, 100003};
static const int Ts[] = {1, 2, 3, 4, 7};

static void test_block(void) {
    for (int i = 0; i < 4; i++) for (int k = 0; k < 5; k++) {
        long work[64] = {0};
        long c = twins_pthreads(Ns[i], Ts[k], MODE_BLOCK, 0, work);
        if (!check_run("block", Ns[i], Ts[k], c, work)) return;
        for (int t = 0; t < Ts[k]; t++)
            if (work[t] != block_work(Ns[i], Ts[k], t)) {
                CHECK(0, "block, N=%ld, %d threads: thread %d did %ld work, but its block costs %ld (is it really a block split?)",
                      Ns[i], Ts[k], t, work[t], block_work(Ns[i], Ts[k], t));
                return;
            }
    }
}

static void test_cyclic(void) {
    for (int i = 0; i < 4; i++) for (int k = 0; k < 5; k++) {
        long work[64] = {0};
        long c = twins_pthreads(Ns[i], Ts[k], MODE_CYCLIC, 0, work);
        if (!check_run("cyclic", Ns[i], Ts[k], c, work)) return;
        for (int t = 0; t < Ts[k]; t++)
            if (work[t] != cyclic_work(Ns[i], Ts[k], t)) {
                CHECK(0, "cyclic, N=%ld, %d threads: thread %d did %ld work, but items t, t+T, ... cost %ld",
                      Ns[i], Ts[k], t, work[t], cyclic_work(Ns[i], Ts[k], t));
                return;
            }
    }
}

static void test_dynamic(void) {
    int chunks[] = {1, 7, 100, 100000};
    for (int i = 0; i < 4; i++) for (int k = 0; k < 5; k++) for (int c = 0; c < 4; c++) {
        long work[64] = {0};
        long got = twins_pthreads(Ns[i], Ts[k], MODE_DYNAMIC, chunks[c], work);
        if (!check_run("dynamic", Ns[i], Ts[k], got, work)) return;
    }
}

static void bench(void) {
    long N = 2000000, work[64];
    printf("\n  N = %ld. Work is counted in trial divisions (the same on every machine).\n", N);
    printf("  Times are for information only; they never decide PASS/FAIL.\n\n");
    printf("  | threads | strategy | imbalance (max/avg work) | time (s) |\n  |--:|---|--:|--:|\n");
    int T_list[] = {2, 4, 8};
    for (int k = 0; k < 3; k++) {
        int T = T_list[k];
        const char *name[] = {"block", "cyclic", "dynamic, chunk 64"};
        for (int m = 0; m < 3; m++) {
            double t0 = bench_now();
            twins_pthreads(N, T, m, 64, work);
            printf("  | %d | %s | %.2f | %.3f |\n", T, name[m], imb(work, T), bench_now() - t0);
        }
    }
    printf("\n  Cyclic with an even number of threads gives the even threads only even numbers,\n"
           "  which are rejected after one test: that is why its imbalance is close to 2.\n");
}

int main(void) {
    harness_watchdog(80);
    harness_hint("each thread: local count and work; block uses the block rule; cyclic p += T; dynamic: atomic_fetch_add(next, chunk)");
    RUN_TEST(test_block);
    RUN_TEST(test_cyclic);
    RUN_TEST(test_dynamic);
    if (h_tests_failed == 0) bench();
    return harness_report();
}
