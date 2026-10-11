// ANSWER - Exercise 2 (core): parallel prefix sum (scan) in two passes   [EXTRA]
// BUILD: -fopenmp
//
// Test:  make verify-answers M=04
// Run:   make run M=04 EX=ex2_prefix_sum ANS=1
#include <omp.h>
#include <stdio.h>
#include <stdlib.h>

void prefix_sum_serial(const long *in, long *out, long n) {
    long run = 0;
    for (long i = 0; i < n; i++) {
        run += in[i];
        out[i] = run;                 // out[i] needs out[i-1]: a loop-carried dependence
    }
}

void prefix_sum_parallel(const long *in, long *out, long n, int nthreads) {
    long offset[257] = {0};           // offset[t] = sum of all blocks before block t
#pragma omp parallel num_threads(nthreads)
    {
        int id = omp_get_thread_num(), p = omp_get_num_threads();
        long first = (long)id * n / p, last = (long)(id + 1) * n / p;

        // Pass 1: each thread scans its own block as if it were alone.
        long run = 0;
        for (long i = first; i < last; i++) {
            run += in[i];
            out[i] = run;
        }
        offset[id + 1] = run;         // the total of my block

        // Wait until every block total is known.
#pragma omp barrier
        // One thread turns the block totals into running offsets:
        // offset[t] = total of blocks 0 .. t-1. This loop has only p steps.
#pragma omp single
        for (int t = 1; t <= p; t++) offset[t] += offset[t - 1];
        // (single ends with an implied barrier, so everyone sees the offsets.)

        // Pass 2: add the total of all earlier blocks to my block.
        for (long i = first; i < last; i++) out[i] += offset[id];
    }
}

#ifndef TESTING
int main(void) {
    long n = 20;
    long in[20], out[20];
    for (long i = 0; i < n; i++) in[i] = i + 1;
    prefix_sum_parallel(in, out, n, 4);
    for (long i = 0; i < n; i++) printf("%ld ", out[i]);
    printf("\n(expected 1 3 6 10 ... 210)\n");
    return 0;
}
#endif
