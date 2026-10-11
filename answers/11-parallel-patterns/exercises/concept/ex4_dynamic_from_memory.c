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

typedef struct {
    int rank;
    long n, chunk;
    pf_body body;
    void *ctx;
    atomic_long *next;
} worker_arg;

static void *worker(void *v) {
    worker_arg *a = v;
    long start;
    while ((start = atomic_fetch_add(a->next, a->chunk)) < a->n) {
        long end = start + a->chunk < a->n ? start + a->chunk : a->n;
        for (long i = start; i < end; i++) a->body(i, a->rank, a->ctx);
    }
    return NULL;
}

void for_dynamic(long n, int p, long chunk, pf_body body, void *ctx) {
    atomic_long next = 0;
    pthread_t *t = malloc(sizeof *t * (size_t)p);
    worker_arg *a = malloc(sizeof *a * (size_t)p);
    for (int r = 0; r < p; r++) {
        a[r] = (worker_arg){r, n, chunk < 1 ? 1 : chunk, body, ctx, &next};
        pthread_create(&t[r], NULL, worker, &a[r]);
    }
    for (int r = 0; r < p; r++) pthread_join(t[r], NULL);
    free(t);
    free(a);
}
