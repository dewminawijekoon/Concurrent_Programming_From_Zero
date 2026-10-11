// break_it/naive_barrier.c - why "one counter + one semaphore" is not a reusable barrier.
//
// Run:   make run M=07 EX=naive_barrier
//
// The naive barrier:
//     wait(mutex); count++;
//     if (count == n) { count = 0; post(gate) n times; }   // last one opens the gate
//     post(mutex);
//     wait(gate);
//
// Used ONCE, it works. Used in a loop, it breaks:
//   1. The last thread of round 1 posts n tokens on `gate`.
//   2. A FAST thread takes one token, runs on, and calls the barrier again
//      for round 2. It counts itself in and waits on `gate`...
//   3. ...where a round-1 token is still lying around, because a SLOW
//      thread has not taken its token yet. The fast thread takes it and
//      goes through round 2's barrier ALONE.
//   4. The slow thread now has no token. It is stuck in round 1.
// This program runs the naive barrier many times and reports what happens.
// The fix is the two-phase barrier of exercises/lab/lab5_reusable_barrier.c.
#define _GNU_SOURCE
#include <errno.h>
#include <pthread.h>
#include <sched.h>
#include <semaphore.h>
#include <stdatomic.h>
#include <stdio.h>
#include <time.h>

#define THREADS 4
#define ROUNDS 200000

static sem_t mutex, gate;
static int count;
static atomic_int arrived[ROUNDS];
static atomic_long too_early;
static atomic_int stuck;      // set when a thread waited a whole second for the gate
static atomic_long first_bad_round = -1;

// returns 0 if we got through, -1 if we gave up after one second
static int naive_barrier_wait(void) {
    sem_wait(&mutex);
    count++;
    if (count == THREADS) {
        count = 0;
        for (int i = 0; i < THREADS; i++) sem_post(&gate);
    }
    sem_post(&mutex);
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += 1;
    while (sem_timedwait(&gate, &ts) != 0)
        if (errno == ETIMEDOUT) return -1;
    return 0;
}

static void *worker(void *arg) {
    long id = (long)arg;
    for (long r = 0; r < ROUNDS && !atomic_load(&stuck); r++) {
        if ((r + id) % 7 == 0) sched_yield();   // make some threads a little slower
        atomic_fetch_add(&arrived[r], 1);
        if (naive_barrier_wait() != 0) {
            atomic_store(&stuck, 1);
            break;
        }
        if (atomic_load(&arrived[r]) != THREADS) {
            long expected = -1;
            atomic_compare_exchange_strong(&first_bad_round, &expected, r + 1);
            atomic_fetch_add(&too_early, 1);
        }
    }
    return NULL;
}

int main(void) {
    pthread_t t[THREADS];
    sem_init(&mutex, 0, 1);
    sem_init(&gate, 0, 0);
    for (long i = 0; i < THREADS; i++) pthread_create(&t[i], NULL, worker, (void *)i);
    for (int i = 0; i < THREADS; i++) pthread_join(t[i], NULL);
    printf("%d threads, up to %d barrier calls with the NAIVE barrier:\n", THREADS, ROUNDS);
    printf("  a thread passed the barrier before everyone arrived: %ld times\n", atomic_load(&too_early));
    if (atomic_load(&first_bad_round) >= 0)
        printf("  first time: barrier call number %ld\n", atomic_load(&first_bad_round));
    if (atomic_load(&stuck))
        printf("  and then a thread waited 1 second at the gate with no token left: the program was stuck.\n");
    if (atomic_load(&too_early) == 0 && !atomic_load(&stuck))
        printf("  nothing went wrong this time - run it again (a lucky run proves nothing).\n");
    else
        printf("  A fast thread took a gate token meant for a slow thread of the previous round.\n"
               "  See the README, 'Reusable barrier', for the two-phase fix.\n");
    return 0;
}
