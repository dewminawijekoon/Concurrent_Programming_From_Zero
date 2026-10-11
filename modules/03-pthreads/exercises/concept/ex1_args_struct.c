// Exercise 1 (warm-up) - give each thread its own struct              [L04 slides 11-13]
//
// PREDICT FIRST: sum_in_blocks() starts thread t with &args[t]. What could
//                go wrong if it passed &t (the address of the loop counter)
//                instead? Run break_it/loop_index_bug.c to check your guess.
//
// sum_in_blocks(n, nthreads, args) adds 0 + 1 + ... + (n-1) using nthreads
// threads. The numbers are cut into nthreads blocks that sit next to each
// other. Thread t handles the numbers first, first+1, ..., last-1 where
//
//     first = t * n / nthreads        last = (t + 1) * n / nthreads
//
// (Example: n = 10, 3 threads -> [0,3) [3,6) [6,10).) This rule works even
// when nthreads does not divide n. The struct is in ex1_args_struct.h.
//
// TODO 1: block_worker - convert arg back to a struct block_arg *, fill in
//         first, last and partial.
// TODO 2: sum_in_blocks - fill in the "in" fields of args[t], start one
//         thread per struct with pthread_create, join them all with
//         pthread_join, and return the sum of the partial sums.
//
// Test:  make test M=03 T=ex1_args_struct
// Run:   make run  M=03 EX=ex1_args_struct ARGS="20 3"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include "ex1_args_struct.h"

void *block_worker(void *arg) {
    // TODO 1
    (void)arg;
    return NULL;
}

long sum_in_blocks(long n, int nthreads, struct block_arg *args) {
    // TODO 2 (you need an array of pthread_t, one per thread)
    (void)n; (void)nthreads; (void)args;
    return 0;
}

#ifndef TESTING
int main(int argc, char *argv[]) {
    long n = argc > 1 ? atol(argv[1]) : 20;
    int p = argc > 2 ? atoi(argv[2]) : 3;
    struct block_arg *args = calloc((size_t)p, sizeof *args);
    long total = sum_in_blocks(n, p, args);
    for (int t = 0; t < p; t++)
        printf("thread %d: numbers %ld..%ld, partial sum %ld\n", t, args[t].first,
               args[t].last - 1, args[t].partial);
    printf("total = %ld (expected %ld)\n", total, n * (n - 1) / 2);
    free(args);
    return 0;
}
#endif
