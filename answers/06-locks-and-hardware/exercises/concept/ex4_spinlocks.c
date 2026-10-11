// ANSWER - Exercise 4 (core) - spin locks built on a hardware instruction
//                     [L08-TLP slides 31-33; L09 slide 3]
//
// Filter and Bakery need n shared variables and many steps. With ONE
// hardware instruction that reads and writes a variable in a single step,
// a lock needs only one variable.
//
//   TEST_AND_SET(x)   sets x to true and returns the OLD value, in one step
//   EXCHANGE(x, v)    writes v into x and returns the OLD value, in one step
//
// (a) TAS lock: keep doing TEST_AND_SET(held) until the old value was false.
//     If the old value was false, the lock was free and now it is yours.
// (b) TTAS lock ("test, then test-and-set"): first wait with plain LOADs
//     until the lock LOOKS free, and only then try TEST_AND_SET. If that
//     fails (someone else got it first), go back to waiting with LOADs.
//
// Both locks are correct. The difference is speed. Every TEST_AND_SET is a
// WRITE, so each try takes the lock's cache line away from all other cores
// (section 5 of the README). A plain LOAD can be answered from the core's
// own cache copy, without disturbing anyone, until the holder unlocks.
//
// PREDICT FIRST: with 8 threads fighting for the lock, which one is faster,
//                TAS or TTAS? With 1 thread? Then run the benchmark.
//
// Test:  make test M=06 T=ex4_spinlocks
// Run:   make run  M=06 EX=ex4_spinlocks     (prints the benchmark table)
#include <pthread.h>
#include <stdio.h>

#include "atomics.h"
#include "ex4_spinlocks.h"

void spin_init(spinlock *l) {
    STORE(l->held, false);
}

void tas_lock(spinlock *l) {
    // Old value true: someone else holds it, try again.
    // Old value false: it was free, and our TEST_AND_SET just took it.
    while (TEST_AND_SET(l->held))
        cpu_relax();
}

void tas_unlock(spinlock *l) {
    STORE(l->held, false);
}

void ttas_lock(spinlock *l) {
    for (;;) {
        // "test": read only. While the lock is held this reads our own
        // cached copy and makes no traffic for the other cores.
        while (LOAD(l->held))
            cpu_relax();
        // "test-and-set": the lock looked free; now really try to take it.
        // Another thread may have been faster, so check the old value.
        if (!TEST_AND_SET(l->held))
            return;
    }
}

void ttas_unlock(spinlock *l) {
    STORE(l->held, false);
}

#ifndef TESTING
#include <stdlib.h>
#include <unistd.h>
#include "bench.h"

static spinlock lk;
static pthread_mutex_t mu = PTHREAD_MUTEX_INITIALIZER;
static long counter, per_thread;
static int which;   // 0 = TAS, 1 = TTAS, 2 = pthread mutex

static void *worker(void *arg) {
    (void)arg;
    for (long k = 0; k < per_thread; k++) {
        if (which == 0) tas_lock(&lk);
        else if (which == 1) ttas_lock(&lk);
        else pthread_mutex_lock(&mu);
        counter++;
        if (which == 0) tas_unlock(&lk);
        else if (which == 1) ttas_unlock(&lk);
        else pthread_mutex_unlock(&mu);
    }
    return NULL;
}

static double run(int kind, int nthreads, long total) {
    pthread_t t[64];
    which = kind;
    per_thread = total / nthreads;
    counter = 0;
    spin_init(&lk);
    double t0 = bench_now();
    for (int i = 0; i < nthreads; i++) pthread_create(&t[i], NULL, worker, NULL);
    for (int i = 0; i < nthreads; i++) pthread_join(t[i], NULL);
    double secs = bench_now() - t0;
    if (counter != per_thread * nthreads)
        printf("  !! %s lost updates: counter %ld, expected %ld\n",
               kind == 0 ? "TAS" : kind == 1 ? "TTAS" : "mutex", counter, per_thread * nthreads);
    return secs;
}

int main(int argc, char **argv) {
    long total = argc > 1 ? atol(argv[1]) : 2000000;   // lock/unlock pairs in total
    int cores = (int)sysconf(_SC_NPROCESSORS_ONLN);
    printf("\n### %ld lock/unlock pairs shared out between the threads (%d cores)\n\n", total, cores);
    printf("| threads | TAS (s) | TTAS (s) | pthread mutex (s) |\n");
    printf("|--------:|--------:|---------:|------------------:|\n");
    for (int n = 1; n <= 16 && n <= 2 * cores; n *= 2)
        printf("| %7d | %7.3f | %8.3f | %17.3f |\n", n, run(0, n, total), run(1, n, total),
               run(2, n, total));
    printf("\nTimes depend on the machine. What to look for:\n"
           " - all three get slower as threads are added: only one thread can hold the lock,\n"
           "   and the lock's cache line must travel from core to core for every hand-over;\n"
           " - with many threads TAS usually falls behind TTAS, because every failed TAS is\n"
           "   a write that steals the cache line from everyone (on some CPUs the gap is small);\n"
           " - the pthread mutex spins a little and then puts waiting threads to sleep, so it\n"
           "   copes best when there are many threads.\n");
    return 0;
}
#endif
