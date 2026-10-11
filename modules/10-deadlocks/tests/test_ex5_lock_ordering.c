// TIMEOUT: 30
#include "harness.h"

void do_job(int j);
extern long uses[3];

static long rounds;
static void worker(int id, void *ctx) {
    (void)ctx;
    for (long k = 0; k < rounds; k++) do_job(id);
}

static void test_three_jobs_finish(void) {
    rounds = harness_iters(30000);
    harness_parallel(3, worker, NULL);
    for (int r = 0; r < 3; r++)
        CHECK(uses[r] == 2 * rounds, "resource %c was used %ld times, expected %ld", 'A' + r,
              uses[r], 2 * rounds);
    printf("  (all jobs finished: no deadlock in %ld rounds - with the lock order fixed, a cycle "
           "cannot form at all)\n", 3 * rounds);
}

int main(void) {
    harness_watchdog(10);
    harness_hint("A timeout here means the jobs deadlocked: job 2 locks C before A. Always lock "
                 "the lower-numbered resource first.");
    RUN_TEST(test_three_jobs_finish);
    return harness_report();
}
