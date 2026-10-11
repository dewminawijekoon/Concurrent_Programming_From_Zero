/*
 * bench.h — timing helpers and the "threads vs time" tables the labs ask for.
 *
 *   double t0 = bench_now();
 *   ... work ...
 *   double secs = bench_now() - t0;
 *
 *   bench_table_header("Global mutex vs local accumulation");
 *   bench_table_row(1, t1, t1);     // threads, time, serial baseline
 *   bench_table_row(4, t4, t1);
 *
 * Output is a Markdown table, so you can paste it straight into a lab report.
 * Benchmarks never decide PASS/FAIL: timings depend on the machine.
 */
#ifndef BENCH_H
#define BENCH_H

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

/* Wall-clock time in seconds (monotonic clock). */
static inline double bench_now(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec * 1e-9;
}

static inline void bench_table_header(const char *title) {
    printf("\n### %s\n\n", title);
    printf("| threads | time (s) | speedup | efficiency |\n");
    printf("|--------:|---------:|--------:|-----------:|\n");
}

/* speedup = T1 / Tp, efficiency = speedup / p (Lecture 02). */
static inline void bench_table_row(int threads, double secs, double serial_secs) {
    double speedup = serial_secs / secs;
    printf("| %7d | %8.4f | %7.2f | %10.2f |\n", threads, secs, speedup,
           speedup / threads);
}

static int bench_cmp_double(const void *a, const void *b) {
    double x = *(const double *)a, y = *(const double *)b;
    return (x > y) - (x < y);
}

/* Median of n samples (sorts the array in place). Medians resist outliers. */
static inline double bench_median(double *samples, int n) {
    qsort(samples, (size_t)n, sizeof *samples, bench_cmp_double);
    return n % 2 ? samples[n / 2] : (samples[n / 2 - 1] + samples[n / 2]) / 2;
}

/* Number of repetitions: BENCH_REPS env var, default `fallback`. */
static inline int bench_reps(int fallback) {
    const char *s = getenv("BENCH_REPS");
    int r = s ? atoi(s) : fallback;
    return r > 0 ? r : fallback;
}

#endif /* BENCH_H */
