// Exercise 2 (core) - Pthreads: block vs cyclic vs dynamic         [L01 slides 17-26; module 03]
//
// PREDICT FIRST: with 4 threads, rank the three strategies from most to least
//                balanced. Then look at the work table the test prints. Were
//                you right about cyclic? (Hint: what is p % 4 for even p?)
//
// twins_pthreads(N, T, mode, chunk, work) counts the twin pairs with T threads
// and returns the count. work[t] must end up holding the work done by thread t
// (add up what tw_item() adds to a LOCAL counter, then store it once).
//   MODE_BLOCK   thread t takes one run of neighbouring items (block_range rule)
//   MODE_CYCLIC  thread t takes items t, t + T, t + 2T, ...
//   MODE_DYNAMIC threads grab the next `chunk` items from a shared counter
//                with an atomic fetch-and-add (L01's shared counter), until
//                the items run out
// Each thread keeps its own count and work and returns them; the main thread
// adds them up after pthread_join. No locks are needed.
//
// Test:  make test M=16 T=ex2_pthreads           (prints work and time tables)
// Run:   make run  M=16 EX=ex2_pthreads ARGS="2000000 4"
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include "twins.h"

enum { MODE_BLOCK, MODE_CYCLIC, MODE_DYNAMIC };

typedef struct {
    int id, T, mode, chunk;
    long M;                    // number of items
    atomic_long *next;         // shared counter for MODE_DYNAMIC
    long count, work;          // results of this thread
} job_t;

static void *worker(void *arg) {
    job_t *j = arg;
    long count = 0, work = 0;
    if (j->mode == MODE_BLOCK) {
        long q = j->M / j->T, extra = j->M % j->T;
        long n = q + (j->id < extra ? 1 : 0);
        long first = j->id * q + (j->id < extra ? j->id : extra);
        for (long p = first; p < first + n; p++) count += tw_item(p, &work);
    } else if (j->mode == MODE_CYCLIC) {
        for (long p = j->id; p < j->M; p += j->T) count += tw_item(p, &work);
    } else {
        for (;;) {
            long start = atomic_fetch_add(j->next, j->chunk);   // take the next chunk, atomically
            if (start >= j->M) break;
            long end = start + j->chunk < j->M ? start + j->chunk : j->M;
            for (long p = start; p < end; p++) count += tw_item(p, &work);
        }
    }
    j->count = count;          // written once, read by main only after pthread_join
    j->work = work;
    return NULL;
}

long twins_pthreads(long N, int T, int mode, int chunk, long *work) {
    pthread_t th[64];
    job_t job[64];
    atomic_long next = 0;
    long M = tw_items(N), total = 0;
    for (int t = 0; t < T; t++) {
        job[t] = (job_t){t, T, mode, chunk, M, &next, 0, 0};
        pthread_create(&th[t], NULL, worker, &job[t]);
    }
    for (int t = 0; t < T; t++) {
        pthread_join(th[t], NULL);
        total += job[t].count;
        work[t] = job[t].work;
    }
    return total;
}

#ifndef TESTING
#include "bench.h"
int main(int argc, char **argv) {
    long N = argc > 1 ? atol(argv[1]) : 2000000;
    int T = argc > 2 ? atoi(argv[2]) : 4;
    const char *name[] = {"block", "cyclic", "dynamic (chunk 100)"};
    long work[64];
    for (int m = 0; m < 3; m++) {
        double t0 = bench_now();
        long c = twins_pthreads(N, T, m, 100, work);
        double s = bench_now() - t0;
        long max = 0, sum = 0;
        for (int t = 0; t < T; t++) { sum += work[t]; if (work[t] > max) max = work[t]; }
        printf("%-20s count = %ld, time %.3f s, imbalance %.2f\n", name[m], c, s, (double)max * T / sum);
    }
    return 0;
}
#endif
