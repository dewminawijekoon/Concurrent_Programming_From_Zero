// Exercise 2 (core) - split the work evenly between threads      [L01 slides 17-19]
//
// PREDICT FIRST: you count the primes from 1 to 2,000,000 with 4 threads.
//                Thread 0 gets 1..500,000 and thread 3 gets 1,500,001..2,000,000.
//                Which thread finds more primes? Which thread does more WORK?
//                (Is a big number harder or easier to test than a small one?)
//
// This is the "split the range ahead of time" idea from L01 slide 18.
//
// Part A - split_range(lo, hi, nthreads, t, &my_lo, &my_hi)
//   Split the numbers lo, lo+1, ..., hi-1 (hi itself is NOT included) into
//   nthreads blocks that follow each other. Thread t gets [my_lo, my_hi).
//   The block sizes must differ by at most 1, and the first blocks get the
//   extra numbers. Example: lo=0, hi=10, nthreads=3 gives
//   [0,4) [4,7) [7,10).
//
// Part B - count_primes_static(limit, nthreads, per_thread)
//   Start nthreads threads. Thread t tests every number in its block of
//   [1, limit+1) with is_prime_counted(), and records in per_thread[t]:
//     .primes     how many primes it found
//     .divisions  how many trial divisions it did (is_prime_counted adds them up)
//   Return the total number of primes.
//   Each thread writes only its own per_thread[t], so no lock is needed.
//
// Test:  make test M=00 T=ex2_split_the_work
// Run:   make run  M=00 EX=ex2_split_the_work ARGS="2000000 4"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "bench.h"
#include "ex2_split_the_work.h"

// Given: trial division. Adds the number of divisions it tried to *divisions.
// Bigger numbers need more divisions, so they cost more time to test.
int is_prime_counted(long x, long *divisions) {
    if (x < 2) return 0;
    for (long d = 2; d * d <= x; d++) {
        (*divisions)++;
        if (x % d == 0) return 0;
    }
    return 1;
}

void split_range(long lo, long hi, int nthreads, int t, long *my_lo, long *my_hi) {
    // TODO: n = hi - lo numbers; base = n / nthreads; extra = n % nthreads.
    //       The first `extra` threads get base + 1 numbers, the rest get base.
    //       Work out where thread t's block starts and ends.
    (void)hi; (void)nthreads; (void)t;
    *my_lo = lo;
    *my_hi = lo;
}

long count_primes_static(long limit, int nthreads, struct thread_stats per_thread[]) {
    if (nthreads < 1 || nthreads > SPLIT_MAX_THREADS) return -1;
    // TODO: start nthreads threads. Thread t calls split_range(1, limit + 1, ...),
    //       tests each number in its block with is_prime_counted(x, &divisions),
    //       and fills per_thread[t].primes and per_thread[t].divisions.
    //       Hint: pass each thread a small struct with t, nthreads, limit and
    //       per_thread. Join them all and return the sum of the primes found.
    (void)limit; (void)per_thread;
    return 0;
}

#ifndef TESTING
int main(int argc, char **argv) {
    long limit = argc > 1 ? atol(argv[1]) : 2000000;
    int nthreads = argc > 2 ? atoi(argv[2]) : 4;
    if (nthreads < 1 || nthreads > SPLIT_MAX_THREADS) nthreads = 4;
    struct thread_stats st[SPLIT_MAX_THREADS] = {{0, 0}};

    printf("This machine has %ld cores online.\n", sysconf(_SC_NPROCESSORS_ONLN));
    double t0 = bench_now();
    long total = count_primes_static(limit, nthreads, st);
    double secs = bench_now() - t0;
    printf("primes from 1 to %ld: %ld (%.3f s with %d threads)\n\n", limit, total, secs, nthreads);

    long most = 0, sum = 0;
    for (int t = 0; t < nthreads; t++) {
        sum += st[t].divisions;
        if (st[t].divisions > most) most = st[t].divisions;
    }
    printf("| thread | range | primes found | divisions (work) |\n|---:|---|---:|---:|\n");
    for (int t = 0; t < nthreads; t++) {
        long lo, hi;
        split_range(1, limit + 1, nthreads, t, &lo, &hi);
        printf("| %d | %ld..%ld | %ld | %ld |\n", t, lo, hi - 1, st[t].primes, st[t].divisions);
    }
    if (sum > 0)
        printf("\nThe busiest thread did %.0f%% of the work (a fair share would be %.0f%%).\n"
               "Everyone else waits for it, so it decides the running time.\n",
               100.0 * most / sum, 100.0 / nthreads);
    return 0;
}
#endif
