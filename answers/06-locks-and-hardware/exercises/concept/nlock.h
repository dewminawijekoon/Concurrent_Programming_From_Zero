// nlock.h - interface for the n-thread locks in this module (Filter, Bakery).
// Shared by ex1_filter.c, ex2_bakery.c and ex3_bakery_from_memory.c.
#ifndef NLOCK_H
#define NLOCK_H

#include <sched.h>

#include "atomics.h"

#define MAX_THREADS 16   // the locks support up to this many threads

// Filter lock (ex1). i is the calling thread's id: 0 .. n-1.
void filter_init(int n);
void filter_lock(int i);
void filter_unlock(int i);

// Bakery lock (ex2, ex3). bakery_lock(i) = bakery_doorway(i) then bakery_wait(i).
// The tests call the two halves separately to check first-come-first-served.
void bakery_init(int n);
void bakery_doorway(int i);   // raise my flag and take a ticket (label)
void bakery_wait(int i);      // wait until nobody with a smaller ticket wants in
void bakery_lock(int i);
void bakery_unlock(int i);

// Busy-wait politely. Call this once in every turn of a waiting loop.
// cpu_relax() tells the CPU "I am spinning". Every 64 turns we also give
// the CPU away (sched_yield), so that when there are more threads than CPU
// cores, the thread holding the lock still gets to run and finish.
static inline void spin_pause(unsigned *spins) {
    cpu_relax();
    if ((++*spins & 63) == 0) sched_yield();
}

#endif
