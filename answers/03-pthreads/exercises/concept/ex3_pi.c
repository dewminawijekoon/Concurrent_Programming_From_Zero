// ANSWER - Exercise 3 (core): estimating pi, three ways to protect the sum   [L04 slides 18-26]
//
// Test:  make verify-answers M=03
// Run:   make run M=03 EX=ex3_pi ANS=1 ARGS="10000000"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include "atomics.h"
#include "bench.h"

// pi = 4 * (1 - 1/3 + 1/5 - 1/7 + ...). Term k is (-1)^k / (2k + 1).
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

// Add up this thread's block of terms in a LOCAL variable and return it.
static double my_block_sum(long id) {
    long first = id * n_terms / n_threads, last = (id + 1) * n_terms / n_threads;
    // The sign of term k depends on k, so a block that starts at an odd k
    // must start with a minus sign.
    double factor = (first % 2 == 0) ? 1.0 : -1.0, s = 0.0;
    for (long k = first; k < last; k++, factor = -factor) s += factor / (2 * k + 1);
    return s;
}

static double run_threads(long n, int p, void *(*worker)(void *)) {
    pthread_t *tid = malloc(sizeof *tid * (size_t)p);
    sum = 0.0;
    n_terms = n;
    n_threads = p;
    STORE(turn, 0);
    // Passing the id as a number cast to a pointer is safe here: the VALUE is
    // copied, so later changes to t do not affect the thread.
    for (long t = 0; t < p; t++) pthread_create(&tid[t], NULL, worker, (void *)t);
    for (int t = 0; t < p; t++) pthread_join(tid[t], NULL);
    free(tid);
    return 4.0 * sum;
}

// Version 1: lock around every single addition. Correct but slow: the
// threads spend most of their time waiting for the lock.
static void *worker_each_term(void *arg) {
    long id = (long)arg;
    long first = id * n_terms / n_threads, last = (id + 1) * n_terms / n_threads;
    double factor = (first % 2 == 0) ? 1.0 : -1.0;
    for (long k = first; k < last; k++, factor = -factor) {
        pthread_mutex_lock(&sum_lock);
        sum += factor / (2 * k + 1);   // critical section: one addition
        pthread_mutex_unlock(&sum_lock);
    }
    return NULL;
}
double pi_mutex_each_term(long n, int nthreads) { return run_threads(n, nthreads, worker_each_term); }

// Version 2: add up privately, then ONE locked addition per thread.
static void *worker_local(void *arg) {
    double mine = my_block_sum((long)arg);   // no lock needed: local variable
    pthread_mutex_lock(&sum_lock);
    sum += mine;
    pthread_mutex_unlock(&sum_lock);
    return NULL;
}
double pi_mutex_local(long n, int nthreads) { return run_threads(n, nthreads, worker_local); }

// Version 3: add up privately, then wait (spinning) until it is my turn.
// Thread 0 adds first, then thread 1, and so on.
static void *worker_busywait(void *arg) {
    long id = (long)arg;
    double mine = my_block_sum(id);
    while (LOAD(turn) != id) cpu_relax();    // busy-wait: keeps using the CPU
    sum += mine;                             // only the thread whose turn it is gets here
    STORE(turn, (int)((id + 1) % n_threads)); // hand the turn to the next thread
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
