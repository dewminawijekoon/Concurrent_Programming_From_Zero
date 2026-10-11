// ANSWER - Lab track 2: fix the shared counter with a mutex, two ways    [Lab 01, Ex 3]
//
// Test:  make verify-answers M=03
// Run:   make run M=03 EX=lab2_mutex_counter ANS=1
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

// Part A: every increment is a critical section of its own.
static void *worker_global_mutex(void *arg) {
    (void)arg;
    for (long i = 0; i < m_per_thread; i++) {
        pthread_mutex_lock(&counter_lock);
        counter++;
        pthread_mutex_unlock(&counter_lock);
    }
    return NULL;
}
long counter_global_mutex(int nthreads, long m) { return run(nthreads, m, worker_global_mutex); }

// Part B: count privately, then ONE locked addition per thread.
static void *worker_local(void *arg) {
    (void)arg;
    long local = 0;                   // on this thread's own stack: no other thread sees it
    for (long i = 0; i < m_per_thread; i++) local++;
    pthread_mutex_lock(&counter_lock);
    counter += local;
    pthread_mutex_unlock(&counter_lock);
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
