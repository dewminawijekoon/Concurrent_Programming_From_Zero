// Lab track 1 - measure a serial part and a parallel part, then estimate p   [Lab 01 Ex 4; L02 slides 4-10]
//
// PREDICT FIRST: a program sorts 10 million numbers (serial) and then sums
//                them in N chunks (parallel). Which phase takes longer? If you
//                double N, which phase gets faster? So roughly what speedup
//                will you see with 8 threads?
//
// This mirrors Lab 01 Ex 4 (written from scratch for this repo). main()
// times both phases for N = 1, 2, 4, 8 and prints the tables for Q4.1-Q4.3.
// The test checks your functions on made-up timings, never on real ones,
// because real timings change from machine to machine.
//
// Part A - parallel_chunk_sum(a, n, nthreads)
//   Split a[0..n-1] into nthreads blocks with the given chunk_bounds(),
//   start one thread per block, let each thread sum its block into its own
//   result slot, join, and return the total. Use long long for sums.
//
// Part B - estimate_p_from_phases(t_serial, t_parallel_1)
//   From a run with ONE thread: t_serial = time of the serial phase,
//   t_parallel_1 = time of the parallel phase. The parallel fraction is
//       p = t_parallel_1 / (t_serial + t_parallel_1)
//   Example: 2 s sorting + 8 s summing -> p = 0.8.
//
// Part C - estimate_p_from_speedup(s, n)
//   You measured a speedup s with n threads. Solve Amdahl's law
//   s = 1 / ((1 - p) + p / n) for p:
//       p = (1 - 1/s) / (1 - 1/n)
//   Example: s = 2.5 with n = 4 -> p = 0.6 / 0.75 = 0.8.
//   Return -1 if n < 2 (one thread tells you nothing about p).
//
// Test:  make test M=02 T=lab1_amdahl_measure
// Run:   make run  M=02 EX=lab1_amdahl_measure                (10 million numbers)
//        make run  M=02 EX=lab1_amdahl_measure ARGS=2000000   (smaller, faster)
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include "bench.h"
#include "lab1_amdahl_measure.h"

// Given: block split, as in module 00 exercise 2.
void chunk_bounds(long n, int nthreads, int t, long *lo, long *hi) {
    long base = n / nthreads, extra = n % nthreads;
    *lo = t * base + (t < extra ? t : extra);
    *hi = *lo + base + (t < extra ? 1 : 0);
}

// Given: Amdahl's law (module 02 exercise 1).
double amdahl_predict(double p, double n) { return 1.0 / ((1.0 - p) + p / n); }

long long parallel_chunk_sum(const int *a, long n, int nthreads) {
    if (nthreads < 1 || nthreads > LAB_MAX_THREADS) return -1;
    // TODO (Part A): one thread per block (use chunk_bounds). Give each thread a
    //       struct with a, lo, hi and its own sum field. Join all, add the sums.
    (void)a; (void)n;
    return 0;
}

double estimate_p_from_phases(double t_serial, double t_parallel_1) {
    // TODO (Part B): t_parallel_1 / (t_serial + t_parallel_1)
    (void)t_serial; (void)t_parallel_1;
    return -2.0;
}

double estimate_p_from_speedup(double s, int n) {
    // TODO (Part C): (1 - 1/s) / (1 - 1/n), or -1 if n < 2
    (void)s; (void)n;
    return -2.0;
}

#ifndef TESTING
static int cmp_int(const void *x, const void *y) {
    int a = *(const int *)x, b = *(const int *)y;
    return (a > b) - (a < b);
}

int main(int argc, char **argv) {
    long n = argc > 1 ? atol(argv[1]) : 10000000;
    int *data = malloc(sizeof *data * (size_t)n), *work = malloc(sizeof *work * (size_t)n);
    if (!data || !work) { fprintf(stderr, "out of memory\n"); return 1; }
    srand(42);
    for (long i = 0; i < n; i++) data[i] = rand() % 1000;

    int ns[] = {1, 2, 4, 8};
    double ts[4], tpar[4];
    printf("### Q4.1: %ld numbers (serial phase: qsort; parallel phase: chunk sums)\n\n", n);
    printf("| N | serial (s) | parallel (s) | total (s) | speedup |\n|---:|---:|---:|---:|---:|\n");
    for (int k = 0; k < 4; k++) {
        for (long i = 0; i < n; i++) work[i] = data[i];   // same unsorted input each time
        double t0 = bench_now();
        qsort(work, (size_t)n, sizeof *work, cmp_int);
        ts[k] = bench_now() - t0;
        // The sum is very fast, so repeat it to get a measurable time.
        int reps = 20;
        long long s = 0;
        t0 = bench_now();
        for (int r = 0; r < reps; r++) s += parallel_chunk_sum(work, n, ns[k]);
        tpar[k] = (bench_now() - t0) / reps;
        (void)s;
        double total = ts[k] + tpar[k];
        printf("| %d | %.4f | %.4f | %.4f | %.2f |\n", ns[k], ts[k], tpar[k], total,
               (ts[0] + tpar[0]) / total);
    }

    double p1 = estimate_p_from_phases(ts[0], tpar[0]);
    double s8 = (ts[0] + tpar[0]) / (ts[3] + tpar[3]);
    double p2 = estimate_p_from_speedup(s8, 8);
    printf("\n### Q4.2: estimate p\n\n");
    printf("from the 1-thread phases: p = %.4f / (%.4f + %.4f) = %.4f\n", tpar[0], ts[0], tpar[0], p1);
    printf("from the measured speedup with 8 threads (%.2f): p = (1 - 1/%.2f) / (1 - 1/8) = %.4f\n",
           s8, s8, p2);
    printf("\n### Q4.3: Amdahl's prediction with p = %.4f\n\n", p1);
    printf("64 processors: %.2f;  infinitely many: %.2f\n", amdahl_predict(p1, 64),
           p1 < 1 ? 1.0 / (1.0 - p1) : 1e300);
    printf("\nThe sort dominates, so p is small and extra threads barely help. To raise p\n"
           "(Q4.4), parallelise the sort itself: sort the N chunks in parallel, then merge.\n");
    free(data);
    free(work);
    return 0;
}
#endif
