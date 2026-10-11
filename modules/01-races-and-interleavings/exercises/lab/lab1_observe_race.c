// Lab track 1 - observe the counter race and summarise it        [Lab 01 Ex 2; L03 slides 37-40]
//
// PREDICT FIRST: 4 threads each add 1 to a shared counter 100,000 times,
//                with no lock. Expected total: 400,000. Over 10 runs, what
//                range of results do you expect? Does using 16 threads make
//                it better or worse?
//
// This mirrors Lab 01 Ex 2 (written from scratch for this repo). The lab
// asks for a table of 10 runs, the range of values, the error percentage
// of each run, and the effect of N. Here you write the code that collects
// those numbers; main() prints the tables for your report.
//
// Part A - race_once(nthreads, m)
//   Set the shared `counter` to 0, start nthreads threads that each run
//   increment_many(m), join them all, and return the final counter.
//   (increment_many is given: it does counter++ m times with NO lock.)
//
// Part B - error_percent(actual, expected)
//   How much was lost, as a percentage of the expected value:
//   100 * (expected - actual) / expected. Example: expected 400000,
//   actual 300000 -> 25.0. Return 0 if expected is 0.
//
// Part C - summarize_runs(results, runs, expected)
//   Fill a struct race_stats: min, max, range (= max - min), mean of the
//   results, mean error percentage, and how many runs lost at least one
//   update (result < expected).
//
// Test:  make test M=01 T=lab1_observe_race
// Run:   make run  M=01 EX=lab1_observe_race          (prints the lab tables)
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "lab1_observe_race.h"

// The race in this file is on purpose. This attribute only tells
// ThreadSanitizer not to report it, so that `make tsan` stays quiet for the
// parts that are meant to be correct.
#define DELIBERATE_RACE __attribute__((no_sanitize("thread")))

// `volatile` makes the compiler really load and store counter on every
// increment. Without it, the optimiser (-O2) may turn the whole loop into
// one "counter += m", and the race becomes rare. (The lab compiles with -O0,
// which has the same effect as volatile here.)
static volatile long counter;

DELIBERATE_RACE static void increment_many(long m) {
    for (long i = 0; i < m; i++)
        counter++;   // load, add 1, store: three steps, NOT atomic
}

static void *increment_thread(void *arg) {
    increment_many(*(long *)arg);
    return NULL;
}

DELIBERATE_RACE long race_once(int nthreads, long m) {
    if (nthreads < 1 || nthreads > RACE_MAX_THREADS) return -1;
    // TODO (Part A): counter = 0; start nthreads threads running
    //       increment_thread (pass &m as the argument), join them all,
    //       and return counter.
    (void)increment_thread;
    return 0;
}

double error_percent(long actual, long expected) {
    // TODO (Part B): 100 * (expected - actual) / expected, or 0 if expected is 0.
    //       Careful: do the division in double, not in long.
    (void)actual; (void)expected;
    return -1.0;
}

struct race_stats summarize_runs(const long results[], int runs, long expected) {
    struct race_stats s = {0};
    // TODO (Part C): fill min, max, range, mean, mean_error_pct, runs_with_loss.
    (void)results; (void)runs; (void)expected;
    return s;
}

#ifndef TESTING
#define M 100000
#define RUNS 10
int main(int argc, char **argv) {
    int n = argc > 1 ? atoi(argv[1]) : 4;
    if (n < 1 || n > RACE_MAX_THREADS) n = 4;
    long expected = (long)n * M, results[RUNS];

    printf("### Q2.1: %d runs with N = %d, M = %d (expected %ld)\n\n", RUNS, n, M, expected);
    printf("| run | actual | lost | error %% |\n|---:|---:|---:|---:|\n");
    for (int r = 0; r < RUNS; r++) {
        results[r] = race_once(n, M);
        printf("| %d | %ld | %ld | %.2f |\n", r + 1, results[r], expected - results[r],
               error_percent(results[r], expected));
    }
    struct race_stats s = summarize_runs(results, RUNS, expected);
    printf("\nmin %ld, max %ld, range %ld, mean error %.2f%%, runs with lost updates: %d of %d\n",
           s.min, s.max, s.range, s.mean_error_pct, s.runs_with_loss, RUNS);

    printf("\n### Q2.3: effect of N (M = %d, %d runs each)\n\n", M, RUNS);
    printf("| N | expected | min | max | mean error %% |\n|---:|---:|---:|---:|---:|\n");
    int ns[] = {2, 4, 8, 16};
    for (int k = 0; k < 4; k++) {
        long e = (long)ns[k] * M;
        for (int r = 0; r < RUNS; r++) results[r] = race_once(ns[k], M);
        s = summarize_runs(results, RUNS, e);
        printf("| %d | %ld | %ld | %ld | %.2f |\n", ns[k], e, s.min, s.max, s.mean_error_pct);
    }
    printf("\nThis machine has %d cores. Threads can only collide when they really run at\n"
           "the same time, so the loss usually grows with N until N passes the core count.\n",
           (int)sysconf(_SC_NPROCESSORS_ONLN));
    return 0;
}
#endif
