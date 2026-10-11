// Interface shared by ex2_parallel_for.c and ex4_dynamic_from_memory.c
#ifndef PARALLEL_FOR_H
#define PARALLEL_FOR_H

// The loop body: do the work for item i. `rank` is the number (0 .. p-1) of
// the thread that runs it, so the body can keep per-thread results.
typedef void (*pf_body)(long i, int rank, void *ctx);

// Run body(i, rank, ctx) once for every i in 0 .. n-1, using p threads.
void for_static_block(long n, int p, pf_body body, void *ctx);   // ex2
void for_static_cyclic(long n, int p, pf_body body, void *ctx);  // ex2
void for_dynamic(long n, int p, long chunk, pf_body body, void *ctx);  // ex2 and ex4

#endif
