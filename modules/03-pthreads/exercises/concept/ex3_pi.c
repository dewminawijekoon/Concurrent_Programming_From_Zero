// Exercise 3 (core) - estimating pi, three ways to protect the sum    [L04 slides 18-26]
//
// pi = 4 * (1 - 1/3 + 1/5 - 1/7 + ...). Term k is (-1)^k / (2k + 1).
//
// PREDICT FIRST: with 2 threads that both do `sum += term` on a shared sum
//                with no protection, will you get the same answer every
//                run? (break_it/pi_race.c shows what really happens.)
//
// The code that starts and joins the threads is given (run_threads). Thread
// `id` handles the terms first .. last-1 with the block rule from exercise 1.
// Careful with the sign: if `first` is odd, the block starts with a minus.
//
// TODO 1: worker_each_term - lock sum_lock around EVERY `sum += term`.
// TODO 2: my_block_sum - add up this thread's terms in a local variable.
// TODO 3: worker_local - call my_block_sum, then add the result to sum
//         inside ONE lock/unlock.
// TODO 4: worker_busywait - call my_block_sum, then busy-wait until
//         LOAD(turn) == id, add to sum, then hand the turn to the next
//         thread: STORE(turn, (id + 1) % n_threads). Call cpu_relax() while
//         you spin.
//
// Test:  make test M=03 T=ex3_pi
// Run:   make run  M=03 EX=ex3_pi ARGS="10000000"     (timing table)
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include "atomics.h"
#include "bench.h"

// Given: the serial version.
double pi_serial(long n) {
    double sum = 0.0, factor = 1.0;
    for (long k = 0; k < n; k++, factor = -factor) sum += factor / (2 * k + 1);
    return 4.0 * sum;
}

// ---- shared state used by all three versions ----
static double sum;                 // the shared sum
static long n_terms;
static int n_threads;
static pthread_mutex_t sum_lock = PTHREAD_MUTEX_INITIALIZER;
static shared_int turn;            // busy-wait version: whose turn is it to add?

// Given: start p threads running `worker`, wait for them, return 4 * sum.
static double run_threads(long n, int p, void *(*worker)(void *)) {
    pthread_t *tid = malloc(sizeof *tid * (size_t)p);
    sum = 0.0;
    n_terms = n;
    n_threads = p;
    STORE(turn, 0);
    for (long t = 0; t < p; t++) pthread_create(&tid[t], NULL, worker, (void *)t);
    for (int t = 0; t < p; t++) pthread_join(tid[t], NULL);
    free(tid);
    return 4.0 * sum;
}

// TODO 2: add up terms first .. last-1 of thread `id` in a local variable.
static double my_block_sum(long id) {
    (void)id;
    return 0.0;
}

// TODO 1
static void *worker_each_term(void *arg) {
    long id = (long)arg;
    (void)id; (void)sum_lock;
    return NULL;
}
double pi_mutex_each_term(long n, int nthreads) { return run_threads(n, nthreads, worker_each_term); }

// TODO 3
static void *worker_local(void *arg) {
    long id = (long)arg;
    (void)id; (void)my_block_sum;
    return NULL;
}
double pi_mutex_local(long n, int nthreads) { return run_threads(n, nthreads, worker_local); }

// TODO 4
static void *worker_busywait(void *arg) {
    long id = (long)arg;
    (void)id;
    return NULL;
}
double pi_busywait(long n, int nthreads) { return run_threads(n, nthreads, worker_busywait); }

#ifndef TESTING
int main(int argc, char *argv[]) {
    long n = argc > 1 ? atol(argv[1]) : 10000000;
    printf("serial: pi ~ %.12f\n", pi_serial(n));
    printf("\n| threads | each term + mutex (s) | local sum + mutex (s) | local sum + busy-wait (s) |\n");
    printf("|--------:|---------------------:|---------------------:|-------------------------:|\n");
    for (int p = 1; p <= 16; p *= 2) {
        double t0 = bench_now();
        double a = pi_mutex_each_term(n / 10, p);   // 10 times fewer terms: it is that slow
        double t1 = bench_now();
        double b = pi_mutex_local(n, p);
        double t2 = bench_now();
        double c = pi_busywait(n, p);
        double t3 = bench_now();
        printf("| %7d | %20.4f | %20.4f | %24.4f |\n", p, (t1 - t0) * 10, t2 - t1, t3 - t2);
        (void)a; (void)b; (void)c;
    }
    printf("\n(the first column ran n/10 terms and was multiplied by 10 to compare)\n");
    return 0;
}
#endif
