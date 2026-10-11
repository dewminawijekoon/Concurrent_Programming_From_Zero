// ANSWER - Exercise 1 (warm-up): give each thread its own struct      [L04 slides 11-13]
//
// Test:  make verify-answers M=03
// Run:   make run M=03 EX=ex1_args_struct ANS=1 ARGS="20 3"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include "ex1_args_struct.h"

void *block_worker(void *arg) {
    // pthread_create hands us a void *. We know it really points to a
    // struct block_arg, so we convert it back.
    struct block_arg *a = arg;

    // Block rule: thread t gets [t*n/p, (t+1)*n/p). Multiply BEFORE dividing,
    // so the blocks fit together exactly even when p does not divide n.
    a->first = (long)a->id * a->n / a->nthreads;
    a->last = (long)(a->id + 1) * a->n / a->nthreads;

    long s = 0;                       // local variable: private to this thread
    for (long i = a->first; i < a->last; i++) s += i;
    a->partial = s;                   // only this thread writes args[id]
    return NULL;
}

long sum_in_blocks(long n, int nthreads, struct block_arg *args) {
    pthread_t *tid = malloc(sizeof *tid * (size_t)nthreads);
    for (int t = 0; t < nthreads; t++) {
        args[t].id = t;
        args[t].nthreads = nthreads;
        args[t].n = n;
        // &args[t] is different for every thread and stays valid until we
        // join, so each thread reads its own, unchanging struct.
        pthread_create(&tid[t], NULL, block_worker, &args[t]);
    }
    long total = 0;
    for (int t = 0; t < nthreads; t++) {
        pthread_join(tid[t], NULL);   // after this, args[t] is finished
        total += args[t].partial;
    }
    free(tid);
    return total;
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
