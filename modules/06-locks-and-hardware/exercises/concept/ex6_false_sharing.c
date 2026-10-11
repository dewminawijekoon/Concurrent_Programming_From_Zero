// Exercise 6 (warm-up) - false sharing: when separate variables still fight
//                        [L08-TLP slide 22; README section 5.4]
//
// Each thread counts its OWN events in its OWN counter. No variable is
// shared, so you would expect perfect speedup. But the CPU's caches do not
// move single variables between cores. They move whole cache lines: blocks
// of 64 bytes. Eight 8-byte counters side by side fill ONE line, so every
// increment by one core takes the line away from all the others. This is
// called false sharing: the threads share a cache line, not any data.
//
// The fix: give each counter its own cache line ("padding").
//
//   packed_counter  (given): 8 bytes, so neighbours share a line
//   padded_counter  (TODO):  the same counter, but exactly one per line
//
// PREDICT FIRST: 4 threads each do 20 million increments on their own
//                counter. How much faster is the padded version? 1x? 2x? 10x?
//
// Tasks
//   (a) Change padded_counter so each one fills (and starts at) its own
//       64-byte cache line. Either write `_Alignas(CACHE_LINE)` before the
//       `value` member, or add an array of filler bytes after it.
//   (b) Write padded_worker(), like packed_worker() but for `padded`.
//
// Test:  make test M=06 T=ex6_false_sharing   (checks the layout and the counts)
// Run:   make run  M=06 EX=ex6_false_sharing  (prints the timing table)
#include <pthread.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "atomics.h"

#define CACHE_LINE 64     // bytes per cache line on almost every current CPU
#define MAX_T 16

typedef struct {
    shared_long value;
} packed_counter;

typedef struct {
    shared_long value;    // TODO (a): make each padded_counter fill its own cache line
} padded_counter;

static packed_counter packed[MAX_T];
static padded_counter padded[MAX_T];
static long iters_each;

static void *packed_worker(void *arg) {
    int id = (int)(long)arg;
    for (long k = 0; k < iters_each; k++)
        FETCH_ADD(packed[id].value, 1);   // only thread `id` touches packed[id]
    return NULL;
}

static void *padded_worker(void *arg) {
    // TODO (b): the same as packed_worker, but on padded[id]
    (void)arg;
    return NULL;
}

// Used by the test.
size_t padded_size(void) { return sizeof(padded_counter); }
size_t padded_gap(void)  { return (size_t)((char *)&padded[1] - (char *)&padded[0]); }
size_t padded_offset_in_line(void) { return (size_t)((uintptr_t)&padded[0] % CACHE_LINE); }

// Runs n threads, each adding `iters` to its own counter. Writes each
// counter's final value into out[0..n-1].
void run_counters(int use_padding, int n, long iters, long *out) {
    pthread_t t[MAX_T];
    iters_each = iters;
    for (int i = 0; i < n; i++) {
        STORE(packed[i].value, 0);
        STORE(padded[i].value, 0);
    }
    for (long i = 0; i < n; i++)
        pthread_create(&t[i], NULL, use_padding ? padded_worker : packed_worker, (void *)i);
    for (int i = 0; i < n; i++) pthread_join(t[i], NULL);
    for (int i = 0; i < n; i++)
        out[i] = use_padding ? LOAD(padded[i].value) : LOAD(packed[i].value);
}

#ifndef TESTING
#include "bench.h"
int main(int argc, char **argv) {
    long iters = argc > 1 ? atol(argv[1]) : 20000000;
    long out[MAX_T];
    printf("sizeof(packed_counter) = %zu bytes, sizeof(padded_counter) = %zu bytes\n",
           sizeof(packed_counter), sizeof(padded_counter));
    printf("\n### %ld increments per thread, each thread on its own counter\n\n", iters);
    printf("| threads | packed (s) | padded (s) | padded is faster by |\n");
    printf("|--------:|-----------:|-----------:|--------------------:|\n");
    for (int n = 1; n <= 8; n *= 2) {
        double t0 = bench_now();
        run_counters(0, n, iters, out);
        double tp = bench_now() - t0;
        t0 = bench_now();
        run_counters(1, n, iters, out);
        double tq = bench_now() - t0;
        printf("| %7d | %10.3f | %10.3f | %18.1fx |\n", n, tp, tq, tp / tq);
    }
    printf("\nWith 1 thread there is nobody to fight with, so both take the same time.\n"
           "With more threads the packed counters share cache lines and slow each other down.\n");
    return 0;
}
#endif
