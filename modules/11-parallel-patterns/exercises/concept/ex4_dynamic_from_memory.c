// Exercise 4 (from memory) - dynamic scheduling with a shared counter
//
// Close ex2 and the notes. Write for_dynamic() from a blank page, including
// starting and joining the threads yourself:
//   p threads; each one repeatedly takes the next chunk of `chunk` items from
//   a shared counter (one atomic step), runs body(i, rank, ctx) for the items
//   in it, and stops when the counter has passed n. Treat chunk < 1 as 1.
// In the exam you may be asked for this as pseudocode, together with WHY it
// balances uneven work better than splitting the range ahead of time.
//
// Test:  make test M=11 T=ex4_dynamic_from_memory
#include <pthread.h>
#include <stdatomic.h>
#include <stdlib.h>

#include "parallel_for.h"

void for_dynamic(long n, int p, long chunk, pf_body body, void *ctx) {
    (void)n; (void)p; (void)chunk; (void)body; (void)ctx;
}
