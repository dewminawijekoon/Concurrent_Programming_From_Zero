// TIMEOUT: 60
// Checks ex4_barrier: nobody leaves a round before everyone arrived, exactly
// one thread per round gets 1, and the barrier works for many rounds in a row.
#include "harness.h"
#include "cv_helpers.h"
#include "ex4_barrier.h"

#define MAX_T 8
#define MAX_ROUNDS 3000
static cv_barrier bar;
static int nthreads, nrounds;
static atomic_int arrived_in[MAX_ROUNDS];  // how many threads reached round r
static atomic_int serial_in[MAX_ROUNDS];   // how many got return value 1 in round r
static atomic_long early;                  // threads that left a round too early
static atomic_long early_round;

static void worker(int id, void *ctx) {
    (void)id; (void)ctx;
    for (int r = 0; r < nrounds; r++) {
        atomic_fetch_add(&arrived_in[r], 1);
        harness_jitter();
        if (cv_barrier_wait(&bar)) atomic_fetch_add(&serial_in[r], 1);
        if (atomic_load(&arrived_in[r]) != nthreads) {
            atomic_fetch_add(&early, 1);
            atomic_store(&early_round, r);
        }
    }
}
static void run_all(void *arg) { (void)arg; harness_parallel(nthreads, worker, NULL); }

static int check(int n, int rounds) {
    nthreads = n;
    nrounds = rounds;
    memset(arrived_in, 0, sizeof arrived_in);
    memset(serial_in, 0, sizeof serial_in);
    atomic_store(&early, 0);
    cv_barrier_init(&bar, n);
    cvt_job *j = cvt_start(run_all, NULL);
    if (!cvt_wait(j, 15.0)) {
        CHECK(0, "%d threads: they got stuck inside cv_barrier_wait(). If the last thread resets "
                 "`arrived` to 0, waiters that re-check `arrived < n` see 0 and sleep again. "
                 "Wait for the generation (round number) to change instead.", n);
        return 0;
    }
    int ok = 1;
    if (atomic_load(&early)) {
        CHECK(0, "%d threads: %ld times a thread left round %ld before all %d had arrived",
              n, atomic_load(&early), atomic_load(&early_round), n);
        ok = 0;
    }
    for (int r = 0; r < rounds && ok; r++)
        if (atomic_load(&serial_in[r]) != 1) {
            CHECK(0, "%d threads, round %d: %d threads got return value 1 (exactly one should)",
                  n, r, atomic_load(&serial_in[r]));
            ok = 0;
        }
    CHECK(ok, "see above");
    return ok;
}

static void test_one_thread(void) { check(1, 10); }

static void test_many_rounds(void) {
    int rounds = (int)harness_iters(MAX_ROUNDS);
    if (rounds > MAX_ROUNDS) rounds = MAX_ROUNDS;
    int ns[] = {2, 3, 4, 8};
    for (int i = 0; i < 4; i++)
        if (!check(ns[i], rounds)) return;
    printf("  (2, 3, 4 and 8 threads x %d rounds: nobody left early - a stress test cannot prove "
           "correctness)\n", rounds);
}

int main(void) {
    harness_watchdog(55);
    harness_hint("Remember the generation when you arrive. The last thread resets arrived, adds 1 "
                 "to generation and broadcasts; the others wait WHILE generation is unchanged.");
    RUN_TEST(test_one_thread);
    RUN_TEST(test_many_rounds);
    return harness_report();
}
