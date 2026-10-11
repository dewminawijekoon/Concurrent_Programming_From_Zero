// TIMEOUT: 60
// Tests for lab track 5: a reusable barrier, and the averaging that uses it.
#include "lab5_reusable_barrier.h"
#include "sem_test_util.h"

#define MAX_ROUNDS 20000
static barrier_t bar;
static int nthreads;
static long rounds;
static atomic_int arrived[MAX_ROUNDS];
static atomic_long early;   // times a thread got past the barrier too soon
static int slow_ms;

static void racer(int id, void *ctx) {
    (void)ctx;
    for (long r = 0; r < rounds; r++) {
        if (r == 0 && id == 0 && slow_ms) sleep_ms(slow_ms);   // one slow thread
        harness_jitter();
        atomic_fetch_add(&arrived[r], 1);
        barrier_wait(&bar);
        int a = atomic_load(&arrived[r]);
        if (a != nthreads && atomic_fetch_add(&early, 1) == 0)
            printf("    thread %d passed barrier call %ld when only %d of %d threads had arrived\n",
                   id, r + 1, a, nthreads);
        fflush(stdout);   // the watchdog may end the program soon after
    }
}

static void run_rounds(int n, long r, int slow) {
    nthreads = n;
    rounds = r;
    slow_ms = slow;
    atomic_store(&early, 0);
    for (long k = 0; k < r; k++) atomic_store(&arrived[k], 0);
    CHECK(barrier_init(&bar, n) == 0, "barrier_init failed");
    harness_parallel(n, racer, NULL);
    barrier_destroy(&bar);
}

// One thread is slow to arrive: the others must wait for it.
static void test_waits_for_slow_thread(void) {
    run_rounds(4, 1, 50);
    CHECK(atomic_load(&early) == 0, "threads went past the barrier before the slow thread arrived");
}

// The same barrier is used many times in a row (this is where a naive
// counter + one semaphore barrier breaks).
static void test_reusable_many_rounds(void) {
    long r = harness_iters(5000);
    if (r > MAX_ROUNDS) r = MAX_ROUNDS;
    run_rounds(6, r, 0);
    CHECK(atomic_load(&early) == 0, "%ld times a thread passed the barrier too soon",
          atomic_load(&early));
    if (atomic_load(&early) == 0)
        printf("  (6 threads x %ld barrier calls: nobody passed too soon - no violation observed)\n", r);
}

// Two threads only, many rounds, more pressure on each gate.
static void test_two_threads(void) {
    long r = harness_iters(10000);
    if (r > MAX_ROUNDS) r = MAX_ROUNDS;
    run_rounds(2, r, 0);
    CHECK(atomic_load(&early) == 0, "%ld times a thread passed the barrier too soon",
          atomic_load(&early));
}

static void fill(double *a, int size) {
    for (int i = 0; i < size; i++) a[i] = (i == 0 || i == size - 1) ? 100.0 : 0.0;
}

static int compare(int size, int iters, int threads) {
    double *ref = malloc(sizeof(double) * (size_t)size);
    double *par = malloc(sizeof(double) * (size_t)size);
    fill(ref, size);
    fill(par, size);
    average_serial(ref, size, iters);
    average_parallel(par, size, iters, threads);
    int bad = -1;
    for (int i = 0; i < size && bad < 0; i++)
        if (par[i] != ref[i]) bad = i;
    CHECK(bad < 0, "SIZE=%d ITER=%d threads=%d: arr[%d] = %.6f, but the serial version gives %.6f",
          size, iters, threads, bad, bad < 0 ? 0 : par[bad], bad < 0 ? 0 : ref[bad]);
    free(ref);
    free(par);
    return bad < 0;
}

// The lab's settings: SIZE=10 ITER=5, then SIZE=100 ITER=20 with 2, 4, 8 threads.
static void test_parallel_average_matches_serial(void) {
    for (int t = 1; t <= 4; t++)
        if (!compare(10, 5, t)) return;
    int counts[] = {2, 4, 8};
    for (int k = 0; k < 3; k++)
        for (int rep = 0; rep < 20; rep++)   // repeat: a missing barrier only fails sometimes
            if (!compare(100, 20, counts[k])) return;
}

int main(void) {
    harness_watchdog(25);
    harness_hint("Two phases. Phase 1: mutex; count++; if count == n post turnstile1 n times; "
                 "mutex; wait turnstile1. Phase 2: the same with count-- / count == 0 / turnstile2. "
                 "In the averaging, call barrier_wait() after computing temp AND after copying back.");
    RUN_TEST(test_waits_for_slow_thread);
    RUN_TEST(test_reusable_many_rounds);
    RUN_TEST(test_two_threads);
    RUN_TEST(test_parallel_average_matches_serial);
    return harness_report();
}
