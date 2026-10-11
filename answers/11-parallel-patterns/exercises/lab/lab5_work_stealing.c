// Lab track 5 - work stealing with per-thread deques           [Lab 06, Ex 5]
//
// PREDICT FIRST: the owner of a deque takes tasks from the BACK, and thieves
//                take from the FRONT. Why not both from the same end? Think
//                about (a) how often the owner and a thief need the same
//                task, and (b) which tasks are still "warm" in the owner's cache.
//
// One shared task queue (lab4) is simple, but every worker locks the SAME
// mutex for every task. With many workers and small tasks, they queue up for
// that lock. Work stealing gives each worker its OWN deque (double-ended
// queue) with its own lock:
//   - a worker pushes and pops tasks at the BACK of its own deque; usually
//     nobody else is touching it, so its lock is almost never contended;
//   - only when its deque is empty does it become a thief: it picks another
//     worker at random and steals ONE task from the FRONT of that deque.
//
// Your tasks:
//   1. deque_pop():   owner side. Take the item at the back (the newest), or
//                     return -1 if the deque is empty.
//   2. deque_steal(): thief side. Take the item at the front (the oldest), or
//                     return -1 if the deque is empty.
//      (push is given. front and back only grow, so there is no wrap-around:
//       the items are items[front .. back-1].)
//   3. ws_run(): p deques; deal the tasks out round-robin (task i goes to
//      deque i % p); start p workers. Worker w: pop from deque w and run the
//      task; when its deque is empty, try to steal from the other deques,
//      starting at a random one and trying each one once. If every steal
//      fails, stop. (This is safe ONLY because tasks never create new tasks
//      here: an empty deque stays empty.) Count successful steals per worker
//      in steals[w] if steals is not NULL.
//
// Test:  make test M=11 T=lab5_work_stealing
// Run:   make run  M=11 EX=lab5_work_stealing
#include <math.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>

#include "bench.h"

// ---------------- given: the same tasks as lab4 --------------------------------
typedef struct {
    int id;
    int difficulty;
    double result;
} task_t;

atomic_int *task_done_count = NULL;   // set by the tests

void do_task(task_t *t) {
    if (task_done_count) atomic_fetch_add(&task_done_count[t->id], 1);
    double x = t->id;
    for (int k = 0; k < t->difficulty; k++) x = sin(x) * cos(x) + 1.0;
    t->result = x;
}

void tasks_generate(task_t *t, int n, unsigned seed, int easy, int hard) {
    for (int i = 0; i < n; i++) {
        seed = seed * 1103515245u + 12345u;
        t[i] = (task_t){i, ((seed >> 16) % 10 == 0) ? hard : easy, 0.0};
    }
}

void run_serial(task_t *t, int n) {
    for (int i = 0; i < n; i++) do_task(&t[i]);
}

// ---------------- the deque ------------------------------------------------------
typedef struct {
    int *items;
    int cap;
    int front, back;          // the items are items[front .. back-1]
    pthread_mutex_t lock;     // one lock PER deque (fine-grained)
} deque_t;

void deque_init(deque_t *d, int cap) {
    d->items = malloc(sizeof(int) * (size_t)(cap > 0 ? cap : 1));
    d->cap = cap;
    d->front = d->back = 0;
    pthread_mutex_init(&d->lock, NULL);
}

void deque_destroy(deque_t *d) {
    free(d->items);
    pthread_mutex_destroy(&d->lock);
}

// Given: owner side, add at the back. (The caller never pushes more than cap items.)
void deque_push(deque_t *d, int v) {
    pthread_mutex_lock(&d->lock);
    d->items[d->back++] = v;
    pthread_mutex_unlock(&d->lock);
}

int deque_pop(deque_t *d) {
    pthread_mutex_lock(&d->lock);
    int v = -1;
    if (d->back > d->front) v = d->items[--d->back];   // newest item
    pthread_mutex_unlock(&d->lock);
    return v;
}

int deque_steal(deque_t *d) {
    pthread_mutex_lock(&d->lock);
    int v = -1;
    if (d->back > d->front) v = d->items[d->front++];  // oldest item
    pthread_mutex_unlock(&d->lock);
    return v;
}

// ---------------- the scheduler ----------------------------------------------------
typedef struct {
    int w, p;
    deque_t *dq;          // all p deques
    task_t *tasks;
    long *steals;         // may be NULL
} ws_arg;

static void *ws_worker(void *v) {
    ws_arg *a = v;
    unsigned seed = 12345u + 977u * (unsigned)a->w;   // private random numbers
    long my_steals = 0;
    for (;;) {
        int t = deque_pop(&a->dq[a->w]);
        if (t < 0 && a->p > 1) {                       // my deque is empty: steal
            int start = (int)(rand_r(&seed) % (unsigned)a->p);
            for (int k = 0; k < a->p && t < 0; k++) {
                int victim = (start + k) % a->p;
                if (victim != a->w) t = deque_steal(&a->dq[victim]);
            }
            if (t >= 0) my_steals++;
        }
        if (t < 0) break;   // every deque was empty when I looked, and none can refill
        do_task(&a->tasks[t]);
    }
    if (a->steals) a->steals[a->w] = my_steals;
    return NULL;
}

void ws_run(task_t *tasks, int n, int p, long *steals) {
    deque_t *dq = malloc(sizeof(deque_t) * (size_t)p);
    ws_arg *args = malloc(sizeof(ws_arg) * (size_t)p);
    pthread_t *th = malloc(sizeof(pthread_t) * (size_t)p);
    for (int w = 0; w < p; w++) deque_init(&dq[w], n / p + 1);
    for (int i = 0; i < n; i++) deque_push(&dq[i % p], i);   // round-robin deal
    for (int w = 0; w < p; w++) {
        args[w] = (ws_arg){w, p, dq, tasks, steals};
        pthread_create(&th[w], NULL, ws_worker, &args[w]);
    }
    for (int w = 0; w < p; w++) pthread_join(th[w], NULL);
    for (int w = 0; w < p; w++) deque_destroy(&dq[w]);
    free(dq);
    free(args);
    free(th);
}

#ifndef TESTING
// Given, for comparison: the same deal, but NO stealing.
static void *nosteal_worker(void *v) {
    ws_arg *a = v;
    int t;
    while ((t = deque_pop(&a->dq[a->w])) >= 0) do_task(&a->tasks[t]);
    return NULL;
}
static void run_no_stealing(task_t *tasks, int n, int p) {
    deque_t dq[64];
    ws_arg args[64];
    pthread_t th[64];
    for (int w = 0; w < p; w++) deque_init(&dq[w], n / p + 1);
    for (int i = 0; i < n; i++) deque_push(&dq[i % p], i);
    for (int w = 0; w < p; w++) {
        args[w] = (ws_arg){w, p, dq, tasks, NULL};
        pthread_create(&th[w], NULL, nosteal_worker, &args[w]);
    }
    for (int w = 0; w < p; w++) pthread_join(th[w], NULL);
    for (int w = 0; w < p; w++) deque_destroy(&dq[w]);
}

#define NT 200
int main(void) {
    task_t t[NT];
    int easy = 4000, hard = 400000;
    tasks_generate(t, NT, 7, easy, hard);
    double t0 = bench_now();
    run_serial(t, NT);
    double ts = bench_now() - t0;
    printf("%d tasks (easy = %d, hard = %d steps). serial: %.4f s\n", NT, easy, hard, ts);
    int counts[] = {1, 2, 4, 8};
    bench_table_header("Round-robin deal, NO stealing (given)");
    for (int k = 0; k < 4; k++) {
        tasks_generate(t, NT, 7, easy, hard);
        t0 = bench_now();
        run_no_stealing(t, NT, counts[k]);
        bench_table_row(counts[k], bench_now() - t0, ts);
    }
    bench_table_header("Work stealing");
    long steals[8];
    for (int k = 0; k < 4; k++) {
        tasks_generate(t, NT, 7, easy, hard);
        t0 = bench_now();
        ws_run(t, NT, counts[k], steals);
        bench_table_row(counts[k], bench_now() - t0, ts);
        long total = 0;
        for (int w = 0; w < counts[k]; w++) total += steals[w];
        printf("|         | (%ld tasks were stolen) |\n", total);
    }
    printf("\nCompare with the shared-queue pool of lab4 (make run M=11 EX=lab4_thread_pool).\n");
    return 0;
}
#endif
