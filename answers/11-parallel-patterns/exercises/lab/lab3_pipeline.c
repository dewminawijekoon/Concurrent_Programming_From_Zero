// Lab track 3 - pipeline: 3 stages, bounded queues, a replicated slow stage
//                                                     [Lab 06, Ex 3; module 08]
//
// PREDICT FIRST: stage 2 is ten times slower than stages 1 and 3. You run
//                2 copies of stage 2. (a) Can the items now reach stage 3 in
//                a different order? (b) Stage 1 must tell stage 2 "no more
//                items" with a POISON item. How many poison items must it
//                send, and how does stage 3 know when to stop?
//
// A pipeline is like an assembly line. Each stage is a thread that does one
// step and passes the item on through a BOUNDED queue (a queue with a
// maximum size: a fast stage blocks when the queue in front of a slow stage
// is full, so memory use stays bounded).
//
//    stage 1 ──q12──> stage 2 (k copies) ──q23──> stage 3
//    make item        heavy work                  finish + emit in order
//
// The queue (bq_*) is given: one mutex and two condition variables, like the
// bounded buffer of module 08. You write pipeline_run():
//   1. Create q12 and q23 with capacity qcap.
//   2. Start 1 stage-1 thread, n_stage2 stage-2 threads, 1 stage-3 thread.
//      stage 1: for seq = 0 .. n-1 put {seq, stage1_make(seq)} into q12,
//               then put n_stage2 POISON items (seq = -1), one per stage-2 thread.
//      stage 2: take an item; if it is POISON, pass it on to q23 and stop;
//               else set value = stage2_heavy(&item, work) and put it into q23.
//      stage 3: take items until it has seen n_stage2 POISON items. For a
//               real item, value = stage3_finish(value). Items can arrive out
//               of order (two stage-2 threads overtake each other), so keep
//               early items in a "waiting" array indexed by seq, and call
//               emit() for seq 0, 1, 2, ... strictly in order.
//   3. Join all threads, destroy the queues, return the number of emitted items.
//
// Test:  make test M=11 T=lab3_pipeline
// Run:   make run  M=11 EX=lab3_pipeline ARGS="2000 20000"   (items, work per item)
#include <math.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>

#include "bench.h"

typedef struct {
    int seq;      // item number 0, 1, 2, ...; POISON_SEQ means "no more items"
    long value;
} item_t;
#define POISON_SEQ (-1)

// ---------------- given: a bounded blocking queue (circular array) ----------
typedef struct {
    item_t *buf;
    int cap, head, count;
    pthread_mutex_t m;
    pthread_cond_t not_full, not_empty;
} bq_t;

void bq_init(bq_t *q, int cap) {
    q->buf = malloc(sizeof(item_t) * (size_t)cap);
    q->cap = cap;
    q->head = q->count = 0;
    pthread_mutex_init(&q->m, NULL);
    pthread_cond_init(&q->not_full, NULL);
    pthread_cond_init(&q->not_empty, NULL);
}
void bq_destroy(bq_t *q) {
    free(q->buf);
    pthread_mutex_destroy(&q->m);
    pthread_cond_destroy(&q->not_full);
    pthread_cond_destroy(&q->not_empty);
}
void bq_put(bq_t *q, item_t it) {             // blocks while the queue is full
    pthread_mutex_lock(&q->m);
    while (q->count == q->cap) pthread_cond_wait(&q->not_full, &q->m);
    q->buf[(q->head + q->count) % q->cap] = it;
    q->count++;
    pthread_cond_signal(&q->not_empty);
    pthread_mutex_unlock(&q->m);
}
item_t bq_take(bq_t *q) {                     // blocks while the queue is empty
    pthread_mutex_lock(&q->m);
    while (q->count == 0) pthread_cond_wait(&q->not_empty, &q->m);
    item_t it = q->buf[q->head];
    q->head = (q->head + 1) % q->cap;
    q->count--;
    pthread_cond_signal(&q->not_full);
    pthread_mutex_unlock(&q->m);
    return it;
}

// ---------------- given: the work of each stage -------------------------------
// The tests set these to watch your pipeline (you do not need to touch them).
atomic_int *stage2_calls = NULL;     // stage2_calls[seq] counts calls per item
pthread_t main_thread;               // stage 2 must not run in this thread
atomic_int stage2_on_main = 0;

long stage1_make(int seq) { return 3L * seq + 11; }

long stage2_heavy(const item_t *it, int work) {
    if (stage2_calls && it->seq >= 0) atomic_fetch_add(&stage2_calls[it->seq], 1);
    if (pthread_equal(pthread_self(), main_thread)) atomic_store(&stage2_on_main, 1);
    double x = (double)it->value;
    for (int k = 0; k < work; k++) x = sin(x) * cos(x) + 1.0;
    return it->value * 2 + (long)(x * 10.0);
}

long stage3_finish(long v) { return v % 100003; }

// The serial version: what the pipeline must produce.
void pipeline_serial(int n, int work, item_t *out) {
    for (int i = 0; i < n; i++) {
        item_t it = {i, stage1_make(i)};
        it.value = stage2_heavy(&it, work);
        it.value = stage3_finish(it.value);
        out[i] = it;
    }
}

// ---------------- your pipeline ------------------------------------------------
typedef struct {
    bq_t q12, q23;
    int n, n_stage2, work;
    item_t *out;      // emitted items, in the order emit() was called
    int emitted;
} pipe_t;

// Given: stage 3 calls this for each finished item, in order of seq.
static void emit(pipe_t *p, item_t it) { p->out[p->emitted++] = it; }

static void *stage1_main(void *arg) {
    pipe_t *p = arg;
    for (int seq = 0; seq < p->n; seq++) bq_put(&p->q12, (item_t){seq, stage1_make(seq)});
    // One poison item for EACH stage-2 thread: each one stops after taking one.
    for (int k = 0; k < p->n_stage2; k++) bq_put(&p->q12, (item_t){POISON_SEQ, 0});
    return NULL;
}

static void *stage2_main(void *arg) {
    pipe_t *p = arg;
    for (;;) {
        item_t it = bq_take(&p->q12);
        if (it.seq == POISON_SEQ) {        // pass the poison on, so stage 3 can count them
            bq_put(&p->q23, it);
            return NULL;
        }
        it.value = stage2_heavy(&it, p->work);
        bq_put(&p->q23, it);
    }
}

static void *stage3_main(void *arg) {
    pipe_t *p = arg;
    // Reorder buffer: items that arrived before their turn wait here.
    item_t *waiting = malloc(sizeof(item_t) * (size_t)(p->n > 0 ? p->n : 1));
    char *have = calloc((size_t)(p->n > 0 ? p->n : 1), 1);
    int next = 0, poisons = 0;
    while (poisons < p->n_stage2) {
        item_t it = bq_take(&p->q23);
        if (it.seq == POISON_SEQ) { poisons++; continue; }
        it.value = stage3_finish(it.value);
        waiting[it.seq] = it;
        have[it.seq] = 1;
        while (next < p->n && have[next]) emit(p, waiting[next++]);   // release in order
    }
    free(waiting);
    free(have);
    return NULL;
}

int pipeline_run(int n, int n_stage2, int qcap, int work, item_t *out) {
    pipe_t p = {.n = n, .n_stage2 = n_stage2, .work = work, .out = out, .emitted = 0};
    bq_init(&p.q12, qcap);
    bq_init(&p.q23, qcap);
    pthread_t t1, t3, *t2 = malloc(sizeof(pthread_t) * (size_t)n_stage2);
    pthread_create(&t1, NULL, stage1_main, &p);
    for (int k = 0; k < n_stage2; k++) pthread_create(&t2[k], NULL, stage2_main, &p);
    pthread_create(&t3, NULL, stage3_main, &p);
    pthread_join(t1, NULL);
    for (int k = 0; k < n_stage2; k++) pthread_join(t2[k], NULL);
    pthread_join(t3, NULL);   // after this, p.emitted is final (join = "I see its writes")
    free(t2);
    bq_destroy(&p.q12);
    bq_destroy(&p.q23);
    return p.emitted;
}

#ifndef TESTING
int main(int argc, char **argv) {
    int n = argc > 1 ? atoi(argv[1]) : 2000;
    int work = argc > 2 ? atoi(argv[2]) : 20000;
    main_thread = pthread_self();
    item_t *ref = malloc(sizeof(item_t) * (size_t)n), *out = malloc(sizeof(item_t) * (size_t)n);
    double t0 = bench_now();
    pipeline_serial(n, work, ref);
    double ts = bench_now() - t0;
    printf("%d items, work %d per item. serial: %.4f s\n", n, work, ts);
    printf("(in the table, 'threads' = number of stage-2 threads; queues hold 16 items)\n");
    bench_table_header("Pipeline: 1 stage-1 thread, k stage-2 threads, 1 stage-3 thread");
    for (int k = 1; k <= 4; k++) {
        t0 = bench_now();
        int got = pipeline_run(n, k, 16, work, out);
        double tp = bench_now() - t0;
        bench_table_row(k, tp, ts);
        int ok = got == n;
        for (int i = 0; ok && i < n; i++) ok = out[i].seq == i && out[i].value == ref[i].value;
        if (!ok) printf("  ^ output differs from the serial version!\n");
    }
    free(ref);
    free(out);
    return 0;
}
#endif
