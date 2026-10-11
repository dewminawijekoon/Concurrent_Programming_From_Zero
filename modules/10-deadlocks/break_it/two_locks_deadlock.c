// break_it/two_locks_deadlock.c - two threads, two locks, opposite order.
//
// Run:   make run M=10 EX=two_locks_deadlock              (opposite order: deadlocks)
//        make run M=10 EX=two_locks_deadlock ARGS=ordered (same order: always finishes)
//
// This is the second case of the exercise on L06 slide 14:
//     thread 1: lock(L1); lock(L2); ...      thread 2: lock(L2); lock(L1); ...
// If thread 1 gets L1 and thread 2 gets L2 at about the same moment, each
// waits for the lock the other holds. All four conditions hold: the locks
// are exclusive (mutual exclusion), each thread keeps one while asking for
// the other (hold and wait), nobody can take a lock away (no preemption),
// and the two waits form a cycle (circular wait).
// The program watches for progress and reports who holds what when it stops.
#include <pthread.h>
#include <sched.h>
#include <stdatomic.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define ROUNDS 1000000
static pthread_mutex_t L1 = PTHREAD_MUTEX_INITIALIZER, L2 = PTHREAD_MUTEX_INITIALIZER;
static int ordered;
static atomic_long rounds_done;
static atomic_int holds[2];      // which lock (1 or 2) each thread holds while waiting, 0 = none

static void *thread_main(void *arg) {
    int me = (int)(long)arg;                       // 0 = thread 1, 1 = thread 2
    pthread_mutex_t *first = &L1, *second = &L2;
    if (me == 1 && !ordered) { first = &L2; second = &L1; }
    for (int k = 0; k < ROUNDS; k++) {
        pthread_mutex_lock(first);
        atomic_store(&holds[me], first == &L1 ? 1 : 2);
        sched_yield();                              // widen the bad window
        pthread_mutex_lock(second);
        atomic_store(&holds[me], 0);
        atomic_fetch_add(&rounds_done, 1);          // "critical section"
        pthread_mutex_unlock(second);
        pthread_mutex_unlock(first);
    }
    return NULL;
}

int main(int argc, char **argv) {
    ordered = argc > 1 && strcmp(argv[1], "ordered") == 0;
    printf("%s order\n", ordered ? "same (L1 then L2 in both threads)" : "opposite");
    pthread_t t[2];
    for (long i = 0; i < 2; i++) pthread_create(&t[i], NULL, thread_main, (void *)i);
    long last = -1;
    for (;;) {
        usleep(300000);
        long now = atomic_load(&rounds_done);
        if (now == 2L * ROUNDS) break;
        if (now == last) {
            printf("DEADLOCK after %ld rounds: thread 1 holds L%d and waits for the other lock;\n"
                   "thread 2 holds L%d and waits for the other lock. Nobody can ever continue.\n"
                   "Try: ARGS=ordered\n", now, atomic_load(&holds[0]), atomic_load(&holds[1]));
            return 1;
        }
        last = now;
    }
    for (int i = 0; i < 2; i++) pthread_join(t[i], NULL);
    printf("finished all %d rounds: with one global lock order, no cycle can form.\n", 2 * ROUNDS);
    return 0;
}
