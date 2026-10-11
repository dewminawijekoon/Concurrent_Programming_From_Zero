// Lab track 4 - master-worker: a thread pool with a shared task queue
//                                  [Lab 06, Ex 4; L01 slides 20-22; module 08]
//
// PREDICT FIRST: 200 tasks; 1 in 10 is 100 times harder than the others.
//                With 4 threads and a STATIC split (50 tasks each), one
//                thread may get many of the hard tasks. How does a shared
//                task queue avoid that?
//
// A thread pool starts a fixed number of worker threads ONCE. Work is put
// into a shared queue; every worker repeatedly takes the next task. Fast
// workers simply take more tasks, so uneven tasks balance out by themselves
// (dynamic load balancing), and we do not pay to create a thread per task.
//
// Part A (exactly Lab 06 Ex 4): run_counter_pool(). The tasks sit in an
//   array. A shared index protected by a mutex says which task is next.
//   Each worker: lock, read and increment the index, unlock; stop if the
//   index is past the end; otherwise do_task() on that task.
//
// Part B (a real pool, like the ones in libraries): tasks can be added at any
//   time, even by other tasks. Implement:
//     pool_create(n)        start n workers that wait for work
//     pool_submit(p, f, a)  add the task "call f(a)" to the queue
//     pool_wait(p)          block until every submitted task has finished
//     pool_destroy(p)       wait for all tasks, stop the workers, free everything
//   Use ONE mutex and two condition variables (see struct pool below).
//   `pending` counts tasks that were submitted but have not finished yet.
//   Increase it in pool_submit (not when a worker starts the task), so
//   pool_wait cannot return while a task that is still running is about to
//   submit more tasks.
//
// Test:  make test M=11 T=lab4_thread_pool
// Run:   make run  M=11 EX=lab4_thread_pool
#include <math.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "bench.h"

// ---------------- given: the tasks ---------------------------------------------
typedef struct {
    int id;
    int difficulty;     // how many steps of work
    double result;
} task_t;

atomic_int *task_done_count = NULL;   // set by the tests: counts do_task calls per id

void do_task(task_t *t) {
    if (task_done_count) atomic_fetch_add(&task_done_count[t->id], 1);
    double x = t->id;
    for (int k = 0; k < t->difficulty; k++) x = sin(x) * cos(x) + 1.0;
    t->result = x;
}

// About 1 task in 10 is `hard`, the rest are `easy` (the same seed gives the same tasks).
void tasks_generate(task_t *t, int n, unsigned seed, int easy, int hard) {
    for (int i = 0; i < n; i++) {
        seed = seed * 1103515245u + 12345u;
        t[i] = (task_t){i, ((seed >> 16) % 10 == 0) ? hard : easy, 0.0};
    }
}

void run_serial(task_t *t, int n) {
    for (int i = 0; i < n; i++) do_task(&t[i]);
}

// Given, for comparison: STATIC split. Thread r does one block of tasks.
typedef struct { task_t *t; int lo, hi; } block_arg;
static void *block_main(void *v) {
    block_arg *a = v;
    for (int i = a->lo; i < a->hi; i++) do_task(&a->t[i]);
    return NULL;
}
void run_static(task_t *t, int n, int p) {
    pthread_t th[64];
    block_arg a[64];
    for (int r = 0; r < p; r++) {
        int q = n / p, rem = n % p;
        a[r].t = t;
        a[r].lo = r * q + (r < rem ? r : rem);
        a[r].hi = a[r].lo + q + (r < rem ? 1 : 0);
        pthread_create(&th[r], NULL, block_main, &a[r]);
    }
    for (int r = 0; r < p; r++) pthread_join(th[r], NULL);
}

// ---------------- Part A: shared index ---------------------------------------------
static int next_task;
static pthread_mutex_t next_lock = PTHREAD_MUTEX_INITIALIZER;
static task_t *a_tasks;
static int a_n;

static void *counter_worker(void *arg) {
    (void)arg;
    // TODO (Part A): loop { lock next_lock; i = next_task++; unlock;
    //                       if i >= a_n stop; do_task(&a_tasks[i]); }
    //       Do the task AFTER unlocking, or the threads take turns doing tasks.
    (void)next_lock;
    return NULL;
}

void run_counter_pool(task_t *t, int n, int p) {
    pthread_t th[64];
    a_tasks = t;
    a_n = n;
    next_task = 0;                    // threads not started yet: no lock needed
    for (int r = 0; r < p; r++) pthread_create(&th[r], NULL, counter_worker, NULL);
    for (int r = 0; r < p; r++) pthread_join(th[r], NULL);
}

// ---------------- Part B: a general thread pool ------------------------------------
typedef struct job {
    void (*fn)(void *);
    void *arg;
    struct job *next;
} job;

typedef struct pool {
    pthread_mutex_t m;
    pthread_cond_t work_ready;   // "the queue is not empty, or we are shutting down"
    pthread_cond_t all_done;     // "pending dropped to 0"
    job *head, *tail;            // FIFO queue of jobs waiting for a worker
    int pending;                 // submitted but not yet finished
    bool shutdown;               // set by pool_destroy: workers exit when the queue is empty
    int nthreads;
    pthread_t *threads;
} pool_t;

static void *pool_worker(void *arg) {
    pool_t *p = arg;
    // TODO (Part B): with p->m locked, loop:
    //   - wait on work_ready while the queue is empty and not shutting down
    //   - if the queue is empty now, we are shutting down: unlock and return
    //   - take the first job off the queue, UNLOCK, run j->fn(j->arg), free(j)
    //   - lock again; pending--; if it reached 0, broadcast all_done
    (void)p;
    return NULL;
}

pool_t *pool_create(int nthreads) {
    // TODO (Part B): calloc a pool_t, init the mutex and both condition
    //       variables, start nthreads threads running pool_worker(p).
    (void)nthreads; (void)pool_worker;
    return NULL;
}

void pool_submit(pool_t *p, void (*fn)(void *), void *arg) {
    // TODO (Part B): malloc a job; under the lock append it at the tail,
    //       pending++, signal work_ready.
    (void)p; (void)fn; (void)arg;
}

void pool_wait(pool_t *p) {
    // TODO (Part B): under the lock, wait on all_done while pending > 0
    (void)p;
}

void pool_destroy(pool_t *p) {
    // TODO (Part B): pool_wait; set shutdown and BROADCAST work_ready (every
    //       sleeping worker must wake up to see it); join all workers; free.
    (void)p;
}

#ifndef TESTING
static void task_fn(void *arg) { do_task(arg); }

#define NT 200
int main(void) {
    task_t t[NT];
    int easy = 4000, hard = 400000;
    tasks_generate(t, NT, 7, easy, hard);
    double t0 = bench_now();
    run_serial(t, NT);
    double ts = bench_now() - t0;
    printf("%d tasks (easy = %d steps, hard = %d steps). serial: %.4f s\n", NT, easy, hard, ts);
    int counts[] = {1, 2, 4, 8};
    const char *names[] = {"static split (given)", "Part A: shared index",
                           "Part B: pool with a task queue"};
    for (int s = 0; s < 3; s++) {
        bench_table_header(names[s]);
        for (int k = 0; k < 4; k++) {
            tasks_generate(t, NT, 7, easy, hard);
            t0 = bench_now();
            if (s == 0) run_static(t, NT, counts[k]);
            if (s == 1) run_counter_pool(t, NT, counts[k]);
            if (s == 2) {
                pool_t *p = pool_create(counts[k]);
                for (int i = 0; i < NT; i++) pool_submit(p, task_fn, &t[i]);
                pool_destroy(p);
            }
            bench_table_row(counts[k], bench_now() - t0, ts);
        }
    }
    return 0;
}
#endif
