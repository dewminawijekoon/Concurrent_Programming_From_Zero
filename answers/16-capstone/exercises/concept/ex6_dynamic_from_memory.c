// Exercise 6 (from memory) - The shared-counter version from a blank page   [L01 slides 20-26]
//
// PREDICT FIRST: the L01 slides' counter.getAndIncrement() must be atomic.
//                What goes wrong if two threads run "temp = next; next = temp + chunk"
//                at the same time?
//
// Without looking at exercise 2, write
//     long twins_dynamic(long N, int T, int chunk, long *work)
// with Pthreads: T threads repeatedly take the next `chunk` items (p = 0 .. N-2)
// from a shared atomic counter, test them with tw_item() from twins.h, and
// stop when the items run out. Return the number of twin pairs; put the work
// done by thread t into work[t]. No locks: only the atomic counter is shared.
//
// Test:  make test M=16 T=ex6_dynamic_from_memory
#include <pthread.h>
#include <stdatomic.h>
#include "twins.h"

typedef struct { atomic_long *next; long M; int chunk; long count, work; } dyn_job;

static void *dyn_worker(void *arg) {
    dyn_job *j = arg;
    long count = 0, work = 0;
    for (;;) {
        long start = atomic_fetch_add(j->next, j->chunk);    // get-and-increment, as one step
        if (start >= j->M) break;
        long end = start + j->chunk < j->M ? start + j->chunk : j->M;
        for (long p = start; p < end; p++) count += tw_item(p, &work);
    }
    j->count = count;
    j->work = work;
    return NULL;
}

long twins_dynamic(long N, int T, int chunk, long *work) {
    pthread_t th[64];
    dyn_job job[64];
    atomic_long next = 0;
    long total = 0;
    for (int t = 0; t < T; t++) {
        job[t] = (dyn_job){&next, tw_items(N), chunk, 0, 0};
        pthread_create(&th[t], NULL, dyn_worker, &job[t]);
    }
    for (int t = 0; t < T; t++) {
        pthread_join(th[t], NULL);
        total += job[t].count;
        work[t] = job[t].work;
    }
    return total;
}
