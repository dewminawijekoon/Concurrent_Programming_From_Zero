// Exercise 4 (challenge) - P-FIB: work and span by formula, then real threads   [L02 slides 14-26]
//
// PREDICT FIRST: P-FIB(4) has work 17 and span 8 (L02 slide 21). What happens
//                to the parallelism (work / span) as n grows: does it stay
//                near 2, grow slowly, or grow very fast?
//
// P-FIB(n) from L02 slide 16:
//     if n <= 1 return n
//     x = spawn P-FIB(n-1)     // may run in parallel with the next line
//     y = P-FIB(n-2)
//     sync                      // wait for the spawned call
//     return x + y
//
// In the DAG (see the lesson, section 3.4), a call with n <= 1 is ONE strand.
// A call with n >= 2 is THREE strands: A (before the spawn), B (the call
// to P-FIB(n-2)), C (after the sync). Each strand costs 1 time unit.
//
// Part A - pfib_work(n) and pfib_span(n), WITHOUT building the DAG.
//   Work: count the strands. work(n) = 1 for n <= 1; otherwise
//         3 + work(n-1) + work(n-2).
//   Span: the longest path. Inside a call with n >= 2 there are two ways
//         from A to C:
//           A -> [all of P-FIB(n-1)] -> C               : 1 + span(n-1) + 1
//           A -> B -> [all of P-FIB(n-2)] -> C          : 2 + span(n-2) + 1
//         span(n) is the longer of the two. span(n) = 1 for n <= 1.
//   Use a loop (not plain recursion: work(40) would take very long).
//   Return long values; pfib_parallelism(n) = work / span as a double.
//
// Part B - pfib_threads(n, cutoff, &spawns)
//   Compute fib(n) the P-FIB way with real threads: "spawn" = pthread_create,
//   "sync" = pthread_join. Creating a thread costs far more than adding two
//   numbers, so below a cutoff use the plain serial version:
//     if n <= cutoff: return the serial fib(n), no threads
//     else: start a thread for P-FIB(n-1), compute P-FIB(n-2) yourself,
//           join the thread, and return the sum.
//   Count every pthread_create in *spawns with atomic_fetch_add (many
//   threads update it at the same time).
//
// Test:  make test M=02 T=ex4_pfib
// Run:   make run  M=02 EX=ex4_pfib ARGS="30 20"
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>

#include "ex4_pfib.h"

long pfib_work(int n) {
    if (n <= 1) return 1;
    long w0 = 1, w1 = 1;                 // work(i-2), work(i-1)
    for (int i = 2; i <= n; i++) {
        long w = 3 + w1 + w0;
        w0 = w1;
        w1 = w;
    }
    return w1;
}

long pfib_span(int n) {
    if (n <= 1) return 1;
    long s0 = 1, s1 = 1;                 // span(i-2), span(i-1)
    for (int i = 2; i <= n; i++) {
        long via_spawn = 2 + s1;         // A, the spawned child, C
        long via_call = 3 + s0;          // A, B, the called child, C
        long s = via_spawn > via_call ? via_spawn : via_call;
        s0 = s1;
        s1 = s;
    }
    return s1;
}

double pfib_parallelism(int n) { return (double)pfib_work(n) / (double)pfib_span(n); }

static long fib_serial(int n) { return n <= 1 ? n : fib_serial(n - 1) + fib_serial(n - 2); }

struct pfib_arg {
    int n, cutoff;
    atomic_long *spawns;
    long result;
};

static void *pfib_thread(void *p) {
    struct pfib_arg *a = p;
    a->result = pfib_threads(a->n, a->cutoff, a->spawns);
    return NULL;
}

long pfib_threads(int n, int cutoff, atomic_long *spawns) {
    if (n <= cutoff || n <= 1) return fib_serial(n);   // too small to be worth a thread
    struct pfib_arg child = {n - 1, cutoff, spawns, 0};
    pthread_t t;
    atomic_fetch_add(spawns, 1);
    if (pthread_create(&t, NULL, pfib_thread, &child) != 0) {   // x = spawn P-FIB(n-1)
        // The system ran out of threads: just do the child's work here.
        child.result = pfib_threads(n - 1, cutoff, spawns);
        return child.result + pfib_threads(n - 2, cutoff, spawns);
    }
    long y = pfib_threads(n - 2, cutoff, spawns);      // y = P-FIB(n-2), in this thread
    pthread_join(t, NULL);                             // sync
    return child.result + y;
}

#ifndef TESTING
#include "bench.h"
int main(int argc, char **argv) {
    int n = argc > 1 ? atoi(argv[1]) : 30;
    int cutoff = argc > 2 ? atoi(argv[2]) : 20;
    printf("| n | work | span | parallelism |\n|---:|---:|---:|---:|\n");
    int ns[] = {2, 3, 4, 5, 10, 20, 30, 40};
    for (int k = 0; k < 8; k++)
        printf("| %d | %ld | %ld | %.2f |\n", ns[k], pfib_work(ns[k]), pfib_span(ns[k]),
               pfib_parallelism(ns[k]));

    atomic_long spawns = 0;
    double t0 = bench_now();
    long f = pfib_threads(n, cutoff, &spawns);
    double tp = bench_now() - t0;
    t0 = bench_now();
    long g = fib_serial(n);
    double t1 = bench_now() - t0;
    printf("\nfib(%d) = %ld (serial says %ld)\n", n, f, g);
    printf("threads: %.3f s with %ld threads created (cutoff %d); serial: %.3f s\n",
           tp, (long)spawns, cutoff, t1);
    printf("Try a cutoff of 2: far more threads, and usually SLOWER - overheads (L02 slide 12).\n");
    return 0;
}
#endif
