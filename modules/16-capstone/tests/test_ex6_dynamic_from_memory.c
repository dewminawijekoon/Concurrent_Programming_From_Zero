// TIMEOUT: 60
#include "capstone_checks.h"

long twins_dynamic(long N, int T, int chunk, long *work);

static void test_dynamic(void) {
    long Ns[] = {5, 100, 1000, 100003};
    int Ts[] = {1, 2, 4, 7}, chunks[] = {1, 10, 1000};
    for (int i = 0; i < 4; i++) for (int k = 0; k < 4; k++) for (int c = 0; c < 3; c++) {
        long work[64] = {0};
        long got = twins_dynamic(Ns[i], Ts[k], chunks[c], work);
        if (!check_run("dynamic", Ns[i], Ts[k], got, work)) return;
    }
    long work[64];
    twins_dynamic(1000000, 4, 64, work);
    printf("    N = 1000000, 4 threads, chunk 64: imbalance %.2f (block would be 1.32)\n", imb(work, 4));
}

int main(void) {
    harness_watchdog(50);
    harness_hint("worker: loop { start = atomic_fetch_add(&next, chunk); if (start >= M) break; ... }; join and add up");
    RUN_TEST(test_dynamic);
    return harness_report();
}
