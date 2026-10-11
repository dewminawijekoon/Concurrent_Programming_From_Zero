// Lab track 2 - four shared counters: no lock, mutex, CAS loop, fetch-and-add
//                                        [Lab 05, Ex 2; L12 slides 3-5, 12-16]
//
// PREDICT FIRST: 4 threads each add 1 a million times. Which versions end at
//                exactly 4 000 000? Which is fastest? In the CAS version,
//                how many CAS attempts FAIL with 1 thread? With 8 threads?
//
//   inc_unsafe  plain counter++          (given) loses updates: a race
//   inc_mutex   lock; counter++; unlock  correct, BLOCKING: a thread that is
//                                        paused while holding the lock stops everyone
//   inc_cas     a CAS loop               correct, LOCK-FREE (not wait-free)
//   inc_faa     one fetch-and-add        correct, WAIT-FREE on hardware that has it
//
// CAS (compare-and-swap, L12 slide 15): CAS(x, expected, new) checks, in ONE
// atomic step, whether x still equals `expected`; if so it writes `new` and
// succeeds, otherwise it changes nothing and fails. A CAS loop:
//     do { old = x; } while (!CAS(x, old, old + 1));
// "Read the value, compute the new value, and write it only if nobody
// changed x in between; otherwise try again."
//
// Your tasks (use the helpers from common/atomics.h: LOAD, CAS, FETCH_ADD):
//   1. inc_mutex(n):  n times: lock counter_mutex, counter_plain++, unlock.
//   2. cas_get_and_increment(c): the L12 slide 16 counter: a CAS loop that adds
//      1 to *c and returns the value BEFORE the increment. Every failed CAS
//      must do FETCH_ADD(cas_failures, 1), so we can see the retries.
//   3. inc_cas(n):  n times cas_get_and_increment(&counter_atomic).
//   4. inc_faa(n):  n times FETCH_ADD(counter_atomic, 1).
//
// Test:  make test M=15 T=lab2_counters
// Run:   make run  M=15 EX=lab2_counters ARGS="1000000"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include "atomics.h"
#include "bench.h"

long counter_plain;                       // for inc_unsafe and inc_mutex
pthread_mutex_t counter_mutex = PTHREAD_MUTEX_INITIALIZER;
shared_long counter_atomic;               // for inc_cas and inc_faa
shared_long cas_failures;                 // how many CAS attempts failed

// Given: broken on purpose. `volatile` makes the compiler really do a load,
// an add and a store each time (otherwise -O2 would add n in one go and hide
// the race). Another thread can run between the load and the store.
void inc_unsafe(long n) {
    volatile long *c = &counter_plain;
    for (long i = 0; i < n; i++) *c = *c + 1;
}

void inc_mutex(long n) {
    // TODO
    (void)n;
}

long cas_get_and_increment(shared_long *c) {
    // TODO: old = LOAD(*c); while CAS(*c, old, old + 1) fails: count the
    //       failure in cas_failures and read *c again. Return old.
    (void)c;
    return 0;
}

void inc_cas(long n) {
    // TODO
    (void)n;
}

void inc_faa(long n) {
    // TODO
    (void)n;
}

#ifndef TESTING
static long per_thread;
static void (*version)(long);
static void *runner(void *v) {
    (void)v;
    version(per_thread);
    return NULL;
}

int main(int argc, char **argv) {
    per_thread = argc > 1 ? atol(argv[1]) : 1000000;
    struct { const char *name; void (*fn)(long); } versions[] = {
        {"unsafe", inc_unsafe}, {"mutex", inc_mutex}, {"CAS loop", inc_cas}, {"fetch-add", inc_faa}};
    int counts[] = {1, 2, 4, 8};
    printf("| version   | threads | counter    | expected   | time (s) | failed CAS | ok? |\n");
    printf("|-----------|--------:|-----------:|-----------:|---------:|-----------:|:---:|\n");
    for (int v = 0; v < 4; v++)
        for (int k = 0; k < 4; k++) {
            int p = counts[k];
            pthread_t th[8];
            counter_plain = 0;
            STORE(counter_atomic, 0);
            STORE(cas_failures, 0);
            version = versions[v].fn;
            double t0 = bench_now();
            for (int i = 0; i < p; i++) pthread_create(&th[i], NULL, runner, NULL);
            for (int i = 0; i < p; i++) pthread_join(th[i], NULL);
            double secs = bench_now() - t0;
            long got = v < 2 ? counter_plain : LOAD(counter_atomic);
            long want = (long)p * per_thread;
            printf("| %-9s | %7d | %10ld | %10ld | %8.4f | %10ld | %3s |\n", versions[v].name, p,
                   got, want, secs, LOAD(cas_failures), got == want ? "yes" : "NO");
        }
    return 0;
}
#endif
