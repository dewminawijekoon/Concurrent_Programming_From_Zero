// TIMEOUT: 60
// Checks lab3_mpmc: every item is consumed exactly once, in every configuration,
// including buffer size 1 and more consumers than items; every thread stops.
#include "harness.h"
#include "cv_helpers.h"

struct mpmc_result {
    long produced, consumed;
    double seconds;
};
struct mpmc_result mpmc_run(int M, int N, int K, int buffer_size);
extern void (*mpmc_on_consume)(int consumer, int item);

#define MAX_P 8
static atomic_int seen[MAX_P][1000];   // seen[p][s]: how often item p*1000+s was consumed
static atomic_long strange;            // items that no producer made
static atomic_long strange_value;

static void record(int consumer, int item) {
    (void)consumer;
    int p = item / 1000, s = item % 1000;
    if (item < 0 || p >= MAX_P) {
        atomic_fetch_add(&strange, 1);
        atomic_store(&strange_value, item);
        return;
    }
    atomic_fetch_add(&seen[p][s], 1);
    if ((s & 15) == 0) harness_jitter();
}

struct cfg { int M, N, K, size; };
static struct cfg current;
static struct mpmc_result result;
static void run_current(void *arg) { (void)arg; result = mpmc_run(current.M, current.N, current.K, current.size); }

static int check_config(struct cfg c) {
    memset(seen, 0, sizeof seen);
    atomic_store(&strange, 0);
    current = c;
    cvt_job *j = cvt_start(run_current, NULL);
    if (!cvt_wait(j, 20.0)) {
        CHECK(0, "M=%d producers, N=%d consumers, K=%d, size %d: the run never finished. "
              "Some thread is asleep forever. Do sleeping consumers get woken (and get -1) "
              "when the last sentinel is taken?", c.M, c.N, c.K, c.size);
        return 0;
    }
    int ok = 1;
    long total = (long)c.M * c.K;
    if (result.consumed != total || result.produced != total) {
        CHECK(0, "M=%d N=%d K=%d size %d: produced %ld, consumed %ld, expected %ld of each",
              c.M, c.N, c.K, c.size, result.produced, result.consumed, total);
        ok = 0;
    }
    if (atomic_load(&strange)) {
        CHECK(0, "M=%d N=%d K=%d size %d: consumers were handed %ld values no producer made "
              "(e.g. %ld). mpmc_get() must never return a sentinel as an item.",
              c.M, c.N, c.K, c.size, atomic_load(&strange), atomic_load(&strange_value));
        ok = 0;
    }
    for (int p = 0; p < c.M && ok; p++)
        for (int s = 0; s < c.K; s++) {
            int n = atomic_load(&seen[p][s]);
            if (n != 1) {
                CHECK(0, "M=%d N=%d K=%d size %d: item %d was consumed %d times (should be exactly once)",
                      c.M, c.N, c.K, c.size, p * 1000 + s, n);
                ok = 0;
                break;
            }
        }
    CHECK(ok, "see above");
    return ok;
}

// The three configurations of Lab 02 Ex 3, with the lab's buffer size 10.
static void test_lab_configurations(void) {
    struct cfg cs[] = {{3, 1, 20, 10}, {1, 3, 20, 10}, {3, 3, 20, 10}};
    for (int i = 0; i < 3; i++)
        if (!check_config(cs[i])) return;
}

// Harder: buffer size 1 and 2, more consumers than items, lopsided mixes.
static void test_harder_configurations(void) {
    int K = (int)harness_iters(1000);
    if (K > 1000) K = 1000;
    struct cfg cs[] = {
        {1, 1, K, 1}, {3, 3, K, 1}, {4, 4, K, 1}, {2, 6, K, 2},
        {6, 2, K, 3}, {1, 5, 2, 10}, {3, 8, 1, 1}, {8, 8, K, 4},
    };
    int n = (int)(sizeof cs / sizeof cs[0]);
    for (int round = 0; round < 3; round++)
        for (int i = 0; i < n; i++)
            if (!check_config(cs[i])) return;
    printf("  (%d configurations, 3 times each: every item consumed exactly once, every thread stopped - "
           "a stress test cannot prove correctness)\n", n);
}

int main(void) {
    harness_watchdog(55);
    harness_hint("Put the sentinel logic inside mpmc_get(): count the -1s you remove, never return "
                 "one as an item, and broadcast on not_empty when the last one is taken, so that "
                 "consumers asleep on an empty buffer wake up and return -1.");
    mpmc_on_consume = record;
    RUN_TEST(test_lab_configurations);
    RUN_TEST(test_harder_configurations);
    return harness_report();
}
