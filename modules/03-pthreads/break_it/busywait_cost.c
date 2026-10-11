// break_it/busywait_cost.c - why busy-waiting gets slow with more threads than cores
//                                                                 [L04 slides 21-26]
// Run:   make run M=03 EX=busywait_cost              (n = 100000 terms)
//        make run M=03 EX=busywait_cost ARGS=20000   (fewer terms, if it is too slow)
//
// Both versions protect EVERY single addition (the slow way, on purpose, so
// the cost of waiting is easy to see):
//   busy-wait: thread t waits in a loop until turn == t, adds, passes the turn on.
//              The threads must go strictly in the order 0, 1, 2, ..., 0, 1, ...
//   mutex:     whichever thread gets the lock adds. Waiting threads sleep.
// With more threads than cores, the thread whose turn it is is often not
// running at all. The threads that are running just spin until the
// operating system pauses them. That is the effect in the L04 slide 26 table.
// A busy-wait run that takes more than 10 seconds is stopped early, and the
// program estimates how long it would have taken.
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "atomics.h"
#include "bench.h"

static long n;
static int p;
static double sum;
static shared_int turn;
static shared_int give_up;           // set when the busy-wait run takes too long
static shared_long rounds_done;      // progress of the busy-wait run
static double deadline;
#define LIMIT 10.0                    // seconds before we stop a busy-wait run
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

static void *busy(void *arg) {
    long id = (long)arg;
    long first = id * n / p, last = (id + 1) * n / p;
    double factor = (first % 2 == 0) ? 1.0 : -1.0;
    // Every thread must take part in every round, so each one runs the same
    // number of rounds: the size of the largest block.
    long rounds = (n + p - 1) / p;
    for (long r = 0; r < rounds; r++) {
        long spins = 0;
        while (LOAD(turn) != id) {                     // spin until it is my turn
            cpu_relax();
            if (++spins % 4096 == 0) {                 // now and then: is it taking too long?
                if (LOAD(give_up)) return NULL;
                if (bench_now() > deadline) { STORE(give_up, 1); return NULL; }
            }
        }
        long k = first + r;
        if (k < last) sum += factor / (2 * k + 1);
        factor = -factor;
        if (id == p - 1) STORE(rounds_done, r + 1);
        STORE(turn, (int)((id + 1) % p));
    }
    return NULL;
}

static void *mutexed(void *arg) {
    long id = (long)arg;
    long first = id * n / p, last = (id + 1) * n / p;
    double factor = (first % 2 == 0) ? 1.0 : -1.0;
    for (long k = first; k < last; k++, factor = -factor) {
        pthread_mutex_lock(&lock);
        sum += factor / (2 * k + 1);
        pthread_mutex_unlock(&lock);
    }
    return NULL;
}

static double run(void *(*fn)(void *), int threads) {
    pthread_t t[64];
    p = threads;
    sum = 0;
    STORE(turn, 0);
    STORE(give_up, 0);
    STORE(rounds_done, 0);
    double t0 = bench_now();
    deadline = t0 + LIMIT;
    for (long i = 0; i < p; i++) pthread_create(&t[i], NULL, fn, (void *)i);
    for (int i = 0; i < p; i++) pthread_join(t[i], NULL);
    return bench_now() - t0;
}

int main(int argc, char *argv[]) {
    n = argc > 1 ? atol(argv[1]) : 100000;
    int cores = (int)sysconf(_SC_NPROCESSORS_ONLN);
    printf("n = %ld terms, this machine has %d cores\n\n", n, cores);
    printf("| threads | busy-wait (s) | mutex (s) |\n|--------:|--------------:|----------:|\n");
    for (int threads = 1; threads <= 2 * cores && threads <= 64; threads *= 2) {
        double b = run(busy, threads);
        int stopped = LOAD(give_up);
        double done = (double)LOAD(rounds_done) / ((n + threads - 1) / threads);
        double m = run(mutexed, threads);
        if (stopped)
            printf("| %7d | stopped after %.0f s with %.1f%% done: about %.0f s in total | %9.4f |\n",
                   threads, LIMIT, 100 * done, done > 0 ? b / done : 1e9, m);
        else
            printf("| %7d | %13.4f | %9.4f |\n", threads, b, m);
        fflush(stdout);
    }
    printf("\nWatch the last rows, where there are more threads than cores.\n");
    return 0;
}
