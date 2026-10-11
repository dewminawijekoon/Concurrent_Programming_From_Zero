// Lab track 2 - fix the shared counter with a mutex, two ways         [Lab 01, Ex 3]
//
// N threads each add 1 to a shared counter M times. Without protection the
// result is usually less than N * M (Lab 01, Ex 2; module 01).
//
// PREDICT FIRST: Part A locks the mutex N * M times, Part B only N times.
//                With N = 8 and M = 100000, how many times faster do you
//                think Part B will be? 2x? 10x? 100x?
//
// TODO A: worker_global_mutex - lock counter_lock around EVERY counter++.
// TODO B: worker_local - count in a local variable, then add it to counter
//         inside ONE lock/unlock at the end.
// The code that starts and joins the threads is given (run).
//
// Test:  make test M=03 T=lab2_mutex_counter
// Run:   make run  M=03 EX=lab2_mutex_counter    (prints the lab's table for N = 1..16)
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include "bench.h"

static long counter;                  // the shared counter
static long m_per_thread;             // how many increments each thread does
static pthread_mutex_t counter_lock = PTHREAD_MUTEX_INITIALIZER;

static long run(int nthreads, long m, void *(*worker)(void *)) {
    pthread_t *tid = malloc(sizeof *tid * (size_t)nthreads);
    counter = 0;
    m_per_thread = m;
    for (int t = 0; t < nthreads; t++) pthread_create(&tid[t], NULL, worker, NULL);
    for (int t = 0; t < nthreads; t++) pthread_join(tid[t], NULL);
    free(tid);
    return counter;
}

// Given: no protection at all (Lab 01, Ex 2). Usually loses updates.
static void *worker_no_lock(void *arg) {
    (void)arg;
    // volatile: really load and store counter on every step, as -O0 code does.
    // Otherwise -O2 may turn the loop into one big addition and hide the race.
    volatile long *c = &counter;
    for (long i = 0; i < m_per_thread; i++) (*c)++;
    return NULL;
}
long counter_no_lock(int nthreads, long m) { return run(nthreads, m, worker_no_lock); }

// TODO A
static void *worker_global_mutex(void *arg) {
    (void)arg; (void)counter_lock;
    return NULL;
}
long counter_global_mutex(int nthreads, long m) { return run(nthreads, m, worker_global_mutex); }

// TODO B
static void *worker_local(void *arg) {
    (void)arg;
    return NULL;
}
long counter_local_sum(int nthreads, long m) { return run(nthreads, m, worker_local); }

#ifndef TESTING
int main(void) {
    const long M = 100000;            // the lab's value
    printf("M = %ld increments per thread\n\n", M);
    printf("| N | expected | no lock (Ex 2) | Part A time (s) | Part B time (s) | A / B |\n");
    printf("|--:|---------:|---------------:|----------------:|----------------:|------:|\n");
    for (int n = 1; n <= 16; n *= 2) {
        long racy = counter_no_lock(n, M);
        double t0 = bench_now();
        long a = counter_global_mutex(n, M);
        double t1 = bench_now();
        long b = counter_local_sum(n, M);
        double t2 = bench_now();
        if (a != n * M || b != n * M) printf("WRONG RESULT: A=%ld B=%ld\n", a, b);
        printf("| %d | %ld | %ld | %.6f | %.6f | %.0f |\n", n, n * M, racy, t1 - t0, t2 - t1,
               (t1 - t0) / (t2 - t1));
    }
    return 0;
}
#endif
