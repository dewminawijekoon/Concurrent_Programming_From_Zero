// Exercise 2 (core) - static vs dynamic load balancing        [L01 slides 17-22]
//
// PREDICT FIRST: we count the primes below N with 4 threads. Testing a big
//                number takes longer than testing a small one. With BLOCK
//                partitioning, which thread finishes last? Does CYCLIC
//                partitioning fix that? What does DYNAMIC do differently?
//
// L01 shows two ways to split "test every number" between threads:
//   - split the range ahead of time (static): simple, but the threads get
//     uneven amounts of WORK even though they get the same number of ITEMS;
//   - a shared counter: every free thread takes the next number (dynamic).
//
// Write three "parallel for" functions. Each one must call body(i, rank, ctx)
// exactly once for every i in 0 .. n-1, from p threads:
//   for_static_block   thread r runs ONE block of consecutive items
//                      (same rule as ex1: the first n % p threads get one extra)
//   for_static_cyclic  thread r runs items r, r+p, r+2p, ...
//   for_dynamic        threads repeatedly grab the next CHUNK of `chunk`
//                      items from a shared atomic counter until none are left
//                      (the L01 "shared counter", but taking a chunk at a time)
//
// run_team() below already starts p threads and waits for them. You write
// the three worker functions and the three for_* functions.
//
// Test:  make test M=11 T=ex2_parallel_for
// Run:   make run  M=11 EX=ex2_parallel_for ARGS="2000000"
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>

#include "bench.h"
#include "parallel_for.h"

// Everything one worker thread needs to know.
typedef struct {
    int rank, p;
    long n, chunk;
    pf_body body;
    void *ctx;
    atomic_long *next;   // for_dynamic: the next item nobody has taken yet
} team_arg;

// Given: start p threads running fn(&args[r]) with args[r].rank = r, then wait for all.
static void run_team(int p, void *(*fn)(void *), team_arg proto) {
    pthread_t *t = malloc(sizeof *t * (size_t)p);
    team_arg *a = malloc(sizeof *a * (size_t)p);
    for (int r = 0; r < p; r++) {
        a[r] = proto;
        a[r].rank = r;
        a[r].p = p;
        pthread_create(&t[r], NULL, fn, &a[r]);
    }
    for (int r = 0; r < p; r++) pthread_join(t[r], NULL);
    free(t);
    free(a);
}

static void *block_worker(void *arg) {
    team_arg *a = arg;
    // Same formula as ex1: the first n % p threads get one extra item.
    long q = a->n / a->p, rem = a->n % a->p;
    long lo = a->rank * q + (a->rank < rem ? a->rank : rem);
    long hi = lo + q + (a->rank < rem ? 1 : 0);
    for (long i = lo; i < hi; i++) a->body(i, a->rank, a->ctx);
    return NULL;
}

static void *cyclic_worker(void *arg) {
    team_arg *a = arg;
    for (long i = a->rank; i < a->n; i += a->p) a->body(i, a->rank, a->ctx);
    return NULL;
}

static void *dynamic_worker(void *arg) {
    team_arg *a = arg;
    for (;;) {
        // Take the next chunk in ONE atomic step. fetch_add returns the old
        // value, so no two threads can ever get the same start.
        long start = atomic_fetch_add(a->next, a->chunk);
        if (start >= a->n) break;               // nothing left
        long end = start + a->chunk < a->n ? start + a->chunk : a->n;
        for (long i = start; i < end; i++) a->body(i, a->rank, a->ctx);
    }
    return NULL;
}

void for_static_block(long n, int p, pf_body body, void *ctx) {
    run_team(p, block_worker, (team_arg){.n = n, .body = body, .ctx = ctx});
}

void for_static_cyclic(long n, int p, pf_body body, void *ctx) {
    run_team(p, cyclic_worker, (team_arg){.n = n, .body = body, .ctx = ctx});
}

void for_dynamic(long n, int p, long chunk, pf_body body, void *ctx) {
    atomic_long next = 0;    // lives on this stack frame until run_team returns
    if (chunk < 1) chunk = 1;
    run_team(p, dynamic_worker,
             (team_arg){.n = n, .chunk = chunk, .body = body, .ctx = ctx, .next = &next});
}

#ifndef TESTING
// ---- Demo: count primes below N, like L01's example ------------------------
#define MAXP 64
typedef struct {
    atomic_long primes;
    long work[MAXP][8];   // divisions done by each thread (padded: one cache line each)
} prime_ctx;

static void prime_body(long i, int rank, void *vctx) {
    prime_ctx *c = vctx;
    long divs = 0;
    int prime = i >= 2;
    for (long d = 2; d * d <= i; d++) {   // trial division: bigger i => more work
        divs++;
        if (i % d == 0) { prime = 0; break; }
    }
    c->work[rank][0] += divs;
    if (prime) atomic_fetch_add(&c->primes, 1);
}

// max work / average work: 1.00 means perfectly balanced.
static double imbalance(prime_ctx *c, int p) {
    long max = 0, sum = 0;
    for (int r = 0; r < p; r++) {
        sum += c->work[r][0];
        if (c->work[r][0] > max) max = c->work[r][0];
    }
    return sum ? (double)max * p / (double)sum : 1.0;
}

int main(int argc, char **argv) {
    long n = argc > 1 ? atol(argv[1]) : 2000000;
    int counts[] = {1, 2, 4, 8};
    const char *names[] = {"static block", "static cyclic", "dynamic, chunk 1000"};
    double t1 = 0;
    for (int s = 0; s < 3; s++) {
        printf("\n%s: count primes below %ld\n", names[s], n);
        bench_table_header(names[s]);
        double imb[4];
        long found = 0;
        for (int k = 0; k < 4; k++) {
            int p = counts[k];
            prime_ctx *c = calloc(1, sizeof *c);
            double t0 = bench_now();
            if (s == 0) for_static_block(n, p, prime_body, c);
            if (s == 1) for_static_cyclic(n, p, prime_body, c);
            if (s == 2) for_dynamic(n, p, 1000, prime_body, c);
            double secs = bench_now() - t0;
            if (s == 0 && k == 0) t1 = secs;
            bench_table_row(p, secs, t1);
            imb[k] = imbalance(c, p);
            found = atomic_load(&c->primes);
            free(c);
        }
        printf("\nprimes found: %ld.  work imbalance (max/average, 1.00 = perfect):", found);
        for (int k = 0; k < 4; k++) printf("  p=%d: %.2f", counts[k], imb[k]);
        printf("\n");
    }
    return 0;
}
#endif
