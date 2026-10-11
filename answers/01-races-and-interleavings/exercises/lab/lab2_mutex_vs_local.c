// Lab track 2 - fix the race: one global mutex vs local sums      [Lab 01 Ex 3; L01 slide 28]
//
// PREDICT FIRST: both versions below give the right answer. Version A locks
//                the mutex for EVERY increment; version B counts in a local
//                variable and locks only once per thread at the end.
//                With 8 threads and 100,000 increments each, how many times
//                is the mutex locked in A? In B? Which one is faster, and
//                by roughly how much?
//
// This mirrors Lab 01 Ex 3 (written from scratch for this repo).
// Use counted_lock() / counted_unlock() instead of pthread_mutex_lock /
// unlock: they do the same thing, but also count how many times the lock
// was taken, so the test can check that you used the lock the right way.
//
// Part A - count_global_mutex(nthreads, m)
//   counter = 0. Start nthreads threads. Each thread does m times:
//   lock, counter++, unlock. Join them and return counter.
//
// Part B - count_local_then_add(nthreads, m)
//   counter = 0. Each thread adds 1 to its OWN local variable m times (no
//   lock: nobody else can see a local variable). At the end it locks ONCE,
//   adds its local total to counter, and unlocks. Return counter.
//
// Test:  make test M=01 T=lab2_mutex_vs_local
// Run:   make run  M=01 EX=lab2_mutex_vs_local          (timing table for Q3.1)
//        make run  M=01 EX=lab2_mutex_vs_local ARGS=forget   (Q3.3: forget to unlock)
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "bench.h"
#include "lab2_mutex_vs_local.h"

static long counter;                                   // shared
static pthread_mutex_t counter_lock = PTHREAD_MUTEX_INITIALIZER;

// ---- given: a mutex that counts how often it is taken -----------------
static atomic_long acquisitions;
static void counted_lock(pthread_mutex_t *mu) {
    pthread_mutex_lock(mu);
    atomic_fetch_add(&acquisitions, 1);
}
static void counted_unlock(pthread_mutex_t *mu) { pthread_mutex_unlock(mu); }
long lock_acquisitions(void) { return atomic_load(&acquisitions); }
void reset_lock_acquisitions(void) { atomic_store(&acquisitions, 0); }
// ------------------------------------------------------------------------

struct job { long m; };

static void *global_worker(void *arg) {
    long m = ((struct job *)arg)->m;
    for (long i = 0; i < m; i++) {
        counted_lock(&counter_lock);     // every single increment takes the lock
        counter++;
        counted_unlock(&counter_lock);
    }
    return NULL;
}

static void *local_worker(void *arg) {
    long m = ((struct job *)arg)->m;
    long mine = 0;                       // private: no other thread can touch it
    for (long i = 0; i < m; i++) mine++;
    counted_lock(&counter_lock);         // one lock per thread, at the very end
    counter += mine;
    counted_unlock(&counter_lock);
    return NULL;
}

static long run_threads(int nthreads, long m, void *(*fn)(void *)) {
    if (nthreads < 1 || nthreads > LAB2_MAX_THREADS) return -1;
    pthread_t t[LAB2_MAX_THREADS];
    struct job j = {m};
    counter = 0;
    for (int i = 0; i < nthreads; i++) pthread_create(&t[i], NULL, fn, &j);
    for (int i = 0; i < nthreads; i++) pthread_join(t[i], NULL);
    return counter;                      // all threads have finished: safe to read
}

long count_global_mutex(int nthreads, long m) { return run_threads(nthreads, m, global_worker); }
long count_local_then_add(int nthreads, long m) { return run_threads(nthreads, m, local_worker); }

#ifndef TESTING
// Q3.3 demo: lock, but never unlock. The next lock() call waits forever.
static void *forgetful_worker(void *arg) {
    (void)arg;
    for (int i = 0; i < 3; i++) {
        pthread_mutex_lock(&counter_lock);
        counter++;
        printf("  thread %lu: increment %d done, but I forgot to unlock\n",
               (unsigned long)pthread_self() % 1000, i + 1);
        fflush(stdout);
        // pthread_mutex_unlock(&counter_lock);   <- missing!
    }
    return NULL;
}

static void on_alarm(int sig) {
    (void)sig;
    static const char msg[] =
        "\nStill stuck after 2 seconds: every other lock() call waits forever for a\n"
        "lock that nobody will ever release. Even the SAME thread gets stuck on its\n"
        "second lock() (a normal mutex does not let its owner lock it twice).\n"
        "This is a deadlock. The program would hang forever, so we stop it here.\n";
    ssize_t r = write(STDOUT_FILENO, msg, sizeof msg - 1);
    (void)r;
    _exit(1);
}

int main(int argc, char **argv) {
    if (argc > 1 && strcmp(argv[1], "forget") == 0) {
        signal(SIGALRM, on_alarm);
        alarm(2);
        pthread_t t[2];
        for (int i = 0; i < 2; i++) pthread_create(&t[i], NULL, forgetful_worker, NULL);
        for (int i = 0; i < 2; i++) pthread_join(t[i], NULL);
        printf("finished (this should not happen)\n");
        return 0;
    }

    long m = argc > 1 ? atol(argv[1]) : 100000;
    int reps = bench_reps(3);
    int ns[] = {1, 2, 4, 8, 16};
    printf("### Q3.1: global mutex (A) vs local sum (B), M = %ld, median of %d runs\n\n", m, reps);
    printf("| N | A: time (s) | A: locks taken | B: time (s) | B: locks taken | A / B |\n");
    printf("|---:|---:|---:|---:|---:|---:|\n");
    for (int k = 0; k < 5; k++) {
        int n = ns[k];
        double ta[16], tb[16];
        long la = 0, lb = 0, ra = 0, rb = 0;
        for (int r = 0; r < reps && r < 16; r++) {
            reset_lock_acquisitions();
            double t0 = bench_now();
            ra = count_global_mutex(n, m);
            ta[r] = bench_now() - t0;
            la = lock_acquisitions();
            reset_lock_acquisitions();
            t0 = bench_now();
            rb = count_local_then_add(n, m);
            tb[r] = bench_now() - t0;
            lb = lock_acquisitions();
        }
        int used = reps < 16 ? reps : 16;
        double ma = bench_median(ta, used), mb = bench_median(tb, used);
        printf("| %d | %.5f | %ld | %.5f | %ld | %.0fx |%s\n", n, ma, la, mb, lb, ma / mb,
               (ra == n * m && rb == n * m) ? "" : "  <- WRONG RESULT");
    }
    printf("\nA takes the lock N*M times and the threads queue up for it (lock contention).\n"
           "B takes it only N times, so the threads almost never wait for each other.\n");
    return 0;
}
#endif
