/*
 * harness.h — a tiny, header-only test harness for concurrent C exercises.
 *
 * Exit codes (understood by tools/runtests.sh):
 *   0  = all checks passed
 *   1  = at least one check failed (or the watchdog fired)
 *   77 = test skipped (e.g. toolchain or CPU feature missing)
 *
 * Typical test file:
 *
 *   #include "harness.h"
 *   #include "../exercises/concept/ex1_peterson.h"
 *
 *   static void test_basic(void) { CHECK_EQ(answer(), 42); }
 *
 *   int main(void) {
 *       harness_watchdog(10);          // hang => "FAIL: timeout"
 *       RUN_TEST(test_basic);
 *       return harness_report();
 *   }
 *
 * IMPORTANT — what a passing concurrency test means:
 *   A stress test that finds no violation does NOT prove your code correct;
 *   it means "no violation observed in N runs". Correctness of protocols is
 *   checked exhaustively by tools/interleave, and data races by `make tsan`.
 */
#ifndef HARNESS_H
#define HARNESS_H

#define _GNU_SOURCE
#include <pthread.h>
#include <sched.h>
#include <signal.h>
#include <stdarg.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

/* ------------------------------------------------------------------ */
/* Counters and reporting                                              */
/* ------------------------------------------------------------------ */

#ifdef __FILE_NAME__
#define H_FILE __FILE_NAME__
#else
#define H_FILE __FILE__
#endif

static int h_checks_failed = 0;
static int h_checks_passed = 0;
static int h_tests_failed = 0;
static int h_tests_run = 0;

#define CHECK(cond, ...)                                                   \
    do {                                                                   \
        if (cond) {                                                        \
            h_checks_passed++;                                             \
        } else {                                                           \
            h_checks_failed++;                                             \
            printf("    FAIL (%s:%d): ", H_FILE, __LINE__);              \
            printf(__VA_ARGS__);                                           \
            printf("\n");                                                  \
            fflush(stdout);                                                \
        }                                                                  \
    } while (0)

#define CHECK_EQ(actual, expected)                                         \
    do {                                                                   \
        long long h_a = (long long)(actual), h_e = (long long)(expected);  \
        CHECK(h_a == h_e, "%s == %lld, expected %lld", #actual, h_a, h_e); \
    } while (0)

#define CHECK_NEAR(actual, expected, eps)                                  \
    do {                                                                   \
        double h_a = (double)(actual), h_e = (double)(expected);           \
        double h_d = h_a > h_e ? h_a - h_e : h_e - h_a;                    \
        CHECK(h_d <= (eps), "%s == %.6f, expected %.6f (+/- %g)", #actual, \
              h_a, h_e, (double)(eps));                                    \
    } while (0)

#define RUN_TEST(fn)                                                       \
    do {                                                                   \
        int h_before = h_checks_failed;                                    \
        printf("  - %s\n", #fn);                                           \
        fflush(stdout);                                                    \
        fn();                                                              \
        h_tests_run++;                                                     \
        if (h_checks_failed != h_before) h_tests_failed++;                 \
    } while (0)

/* Print a hint for students, shown only when something failed. */
static const char *h_hint = NULL;
static inline void harness_hint(const char *hint) { h_hint = hint; }

static inline int harness_report(void) {
    if (h_tests_failed == 0) {
        printf("  PASS: %d/%d tests (%d checks)\n", h_tests_run, h_tests_run,
               h_checks_passed);
        return 0;
    }
    printf("  FAIL: %d of %d tests failed\n", h_tests_failed, h_tests_run);
    if (h_hint) printf("  hint: %s\n", h_hint);
    return 1;
}

/* Skip the whole test program (exit code 77). */
static inline void harness_skip(const char *why) {
    printf("  SKIP: %s\n", why);
    fflush(stdout);
    exit(77);
}

/* ------------------------------------------------------------------ */
/* Watchdog: turn a hang into a clear failure                          */
/* ------------------------------------------------------------------ */

static void h_on_alarm(int sig) {
    (void)sig;
    static const char msg[] =
        "    FAIL: timeout - the program stopped making progress.\n"
        "          Likely a deadlock (threads waiting for each other),\n"
        "          a livelock, a lost wake-up, or a TODO left unimplemented.\n";
    ssize_t r = write(STDOUT_FILENO, msg, sizeof msg - 1);
    (void)r;
    _exit(1);
}

/* Fail the test if it is still running after `seconds`. */
static inline void harness_watchdog(unsigned seconds) {
    signal(SIGALRM, h_on_alarm);
    alarm(seconds);
}

/* ------------------------------------------------------------------ */
/* Stress helpers                                                      */
/* ------------------------------------------------------------------ */

/* Scale iteration counts with the STRESS_SCALE env var (default 1). */
static inline long harness_iters(long base) {
    const char *s = getenv("STRESS_SCALE");
    double k = s ? atof(s) : 1.0;
    if (k <= 0) k = 1.0;
    long n = (long)(base * k);
    return n < 1 ? 1 : n;
}

/* Cheap per-thread pseudo-random numbers (xorshift). */
static inline unsigned harness_rand(void) {
    static _Thread_local unsigned s = 0;
    if (s == 0) s = (unsigned)(uintptr_t)&s ^ (unsigned)time(NULL) ^ 0x9e3779b9u;
    s ^= s << 13;
    s ^= s >> 17;
    s ^= s << 5;
    return s;
}

/*
 * Randomly give up the CPU. Sprinkle inside critical sections or between a
 * load and a store to make bad interleavings far more likely to show up.
 */
static inline void harness_jitter(void) {
    if ((harness_rand() & 7) == 0) sched_yield();
}

/* Run fn(id, ctx) on n threads and wait for all of them. */
typedef void (*harness_worker_fn)(int id, void *ctx);

struct h_thread_arg {
    harness_worker_fn fn;
    void *ctx;
    int id;
};

static void *h_thread_main(void *p) {
    struct h_thread_arg *a = p;
    a->fn(a->id, a->ctx);
    return NULL;
}

static inline void harness_parallel(int n, harness_worker_fn fn, void *ctx) {
    pthread_t *t = malloc(sizeof *t * (size_t)n);
    struct h_thread_arg *a = malloc(sizeof *a * (size_t)n);
    for (int i = 0; i < n; i++) {
        a[i] = (struct h_thread_arg){fn, ctx, i};
        if (pthread_create(&t[i], NULL, h_thread_main, &a[i]) != 0) {
            perror("pthread_create");
            exit(1);
        }
    }
    for (int i = 0; i < n; i++) pthread_join(t[i], NULL);
    free(t);
    free(a);
}

/* ------------------------------------------------------------------ */
/* Critical-section monitor: detects mutual-exclusion violations       */
/* ------------------------------------------------------------------ */

typedef struct {
    atomic_int inside;      /* threads currently inside the CS        */
    atomic_int max_inside;  /* largest occupancy ever observed        */
    atomic_long violations; /* entries that found someone else inside */
    atomic_long entries;
} harness_cs_monitor;

#define HARNESS_CS_MONITOR_INIT {0, 0, 0, 0}

static inline void harness_cs_enter(harness_cs_monitor *m) {
    int now = atomic_fetch_add(&m->inside, 1) + 1;
    atomic_fetch_add(&m->entries, 1);
    if (now > 1) atomic_fetch_add(&m->violations, 1);
    int old = atomic_load(&m->max_inside);
    while (now > old && !atomic_compare_exchange_weak(&m->max_inside, &old, now)) {
    }
}

static inline void harness_cs_exit(harness_cs_monitor *m) {
    atomic_fetch_sub(&m->inside, 1);
}

/* ------------------------------------------------------------------ */
/* Misc                                                                */
/* ------------------------------------------------------------------ */

static inline int harness_ncpus(void) {
    long n = sysconf(_SC_NPROCESSORS_ONLN);
    return n > 0 ? (int)n : 1;
}

#endif /* HARNESS_H */
