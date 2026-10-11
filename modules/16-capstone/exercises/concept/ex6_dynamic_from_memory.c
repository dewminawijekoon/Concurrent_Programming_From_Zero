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

long twins_dynamic(long N, int T, int chunk, long *work) {
    // TODO
    (void)N; (void)T; (void)chunk; (void)work; (void)tw_item;
    return 0;
}
