// TIMEOUT: 60
#include "harness.h"

typedef struct {
    int n, k;
    void (*critical)(int id, void *ctx);
    void *ctx;
} ring_cfg;
void ring_run(const ring_cfg *cfg);

static harness_cs_monitor mon = HARNESS_CS_MONITOR_INIT;
static int count[16];
static int order[4096];
static atomic_int norder;

static void cs(int id, void *ctx) {
    (void)ctx;
    harness_cs_enter(&mon);
    int pos = atomic_fetch_add(&norder, 1);
    if (pos < 4096) order[pos] = id;
    harness_jitter();
    count[id]++;                  // plain int: only safe if mutual exclusion holds (make tsan checks it)
    harness_jitter();
    harness_cs_exit(&mon);
}

static void check_ring(int n, int k) {
    memset(count, 0, sizeof count);
    atomic_store(&norder, 0);
    atomic_store(&mon.violations, 0);
    ring_cfg cfg = {n, k, cs, NULL};
    ring_run(&cfg);
    CHECK(atomic_load(&mon.violations) == 0, "n=%d: %ld times two nodes were in the critical section together",
          n, (long)atomic_load(&mon.violations));
    for (int i = 0; i < n; i++) CHECK(count[i] == k, "n=%d: node %d entered %d times, expected %d", n, i, count[i], k);
    int total = atomic_load(&norder), wrong = 0;
    for (int e = 0; e < total; e++) wrong += order[e] != e % n;
    CHECK(total == n * k && wrong == 0, "n=%d: nodes did not enter in ring order 0, 1, ..., %d, 0, 1, ... (%d out of place)", n, n - 1, wrong);
}

static void test_rings(void) {
    int sizes[] = {1, 2, 3, 5, 8};
    for (int s = 0; s < 5; s++) check_ring(sizes[s], 40);
    printf("    no violation observed in %ld entries (the token makes it impossible: there is only one)\n",
           (long)atomic_load(&mon.entries));
}

int main(void) {
    harness_watchdog(40);
    harness_hint("loop { t = mailbox_get(id); if remaining == 0 {pass on unless you zeroed it; return;} if you want: CS, left--, remaining--; pass on }");
    RUN_TEST(test_rings);
    return harness_report();
}
