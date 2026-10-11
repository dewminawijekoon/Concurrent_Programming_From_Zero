// Exercise 3 (core) - the shared counter from the prime example    [L01 slides 17-28]
//
// PREDICT FIRST: in L01's prime example every thread repeatedly asks a shared
//                counter for "the next number to test". If getAndIncrement()
//                is not atomic, what can go wrong: will some numbers be tested
//                twice, skipped, or both?
//
// Module 00 split the range into fixed blocks, and the last thread got the
// most work. Here we use DYNAMIC load balancing (L01 slides 20-22): a thread
// that finishes a number simply takes the next one from the shared counter.
// Fast threads take more numbers, so nobody sits idle while others work.
//
// Part A - get_and_increment(c)
//   Return the counter's current value and add 1 to it, as ONE indivisible
//   step (L01 slides 26-28). The given code does it in two steps with a
//   race_window() in between. Fix it with the mutex inside struct counter.
//   Keep the race_window() call inside the locked part.
//
// Part B - count_primes_dynamic(limit, nthreads, per_thread)
//   Make a counter that starts at 1. Start nthreads threads. Each thread
//   loops:  j = get_and_increment(&counter);  stop if j > limit;
//           otherwise count j as taken and test it with is_prime_counted().
//   Thread t writes its results into per_thread[t] (.taken, .primes,
//   .divisions). Return the total number of primes.
//
// Test:  make test M=01 T=ex3_shared_counter_primes
// Run:   make run  M=01 EX=ex3_shared_counter_primes ARGS="300000 4"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include "ex3_shared_counter_primes.h"
#include "race_window.h"

// Given: trial division; adds the number of divisions it tried to *divisions.
int is_prime_counted(long x, long *divisions) {
    if (x < 2) return 0;
    for (long d = 2; d * d <= x; d++) {
        (*divisions)++;
        if (x % d == 0) return 0;
    }
    return 1;
}

void counter_init(struct counter *c, long start) {
    c->value = start;
    pthread_mutex_init(&c->lock, NULL);
}

long get_and_increment(struct counter *c) {
    // TODO (Part A): make the read and the write ONE indivisible step by
    //       holding c->lock around both. Keep race_window() between them.
    long temp = c->value;            // read
    race_window();                   // another thread can read the same value here
    c->value = temp + 1;             // write
    return temp;
}

long count_primes_dynamic(long limit, int nthreads, struct prime_stats per_thread[]) {
    if (nthreads < 1 || nthreads > PRIMES_MAX_THREADS) return -1;
    // TODO (Part B): struct counter c; counter_init(&c, 1);
    //       start nthreads threads; each one loops:
    //           x = get_and_increment(&c); if (x > limit) stop;
    //           taken++; if (is_prime_counted(x, &divisions)) primes++;
    //       and finally writes taken, primes, divisions into per_thread[t].
    //       Join them all and return the total number of primes.
    (void)limit; (void)per_thread;
    return 0;
}

#ifndef TESTING
int main(int argc, char **argv) {
    long limit = argc > 1 ? atol(argv[1]) : 300000;
    int n = argc > 2 ? atoi(argv[2]) : 4;
    if (n < 1 || n > PRIMES_MAX_THREADS) n = 4;
    struct prime_stats st[PRIMES_MAX_THREADS];
    long total = count_primes_dynamic(limit, n, st);
    printf("primes from 1 to %ld: %ld\n\n", limit, total);
    printf("| thread | numbers taken | primes | divisions (work) |\n|---:|---:|---:|---:|\n");
    long sum = 0;
    for (int t = 0; t < n; t++) {
        printf("| %d | %ld | %ld | %ld |\n", t, st[t].taken, st[t].primes, st[t].divisions);
        sum += st[t].taken;
    }
    printf("\nnumbers taken in total: %ld (should be exactly %ld: each number once)\n", sum, limit);
    printf("Compare the work column with module 00 exercise 2: here it is much more even,\n"
           "because a thread that finishes early just takes more numbers.\n");
    return 0;
}
#endif
