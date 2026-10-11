// ANSWER - Exercise 5 (core) - break circular wait by ordering the locks
//                     [L11 slides 6, 23; L06 slide 14]
//
// Three resources A, B, C, each protected by a pthread mutex. Three jobs
// run over and over, each on its own thread, and each needs TWO resources:
//
//     job 0 needs A and B        job 1 needs B and C        job 2 needs C and A
//
// The code below locks "the first resource the job needs, then the second".
// That gives job 2 the order C then A, while job 0 uses A then B and job 1
// uses B then C. Picture the moment when each job holds its first lock:
//
//     job 0 holds A, waits for B  (held by job 1)
//     job 1 holds B, waits for C  (held by job 2)
//     job 2 holds C, waits for A  (held by job 0)      -> circular wait: deadlock
//
// All four deadlock conditions hold. The easiest one to break is circular
// wait: give every resource a number and ALWAYS lock the lower number first.
// Then nobody can hold a high-numbered lock while waiting for a low one, so
// no cycle can form.
//
// PREDICT FIRST: run the unchanged code (make run M=10 EX=ex5_lock_ordering).
//                How quickly does it stop? Which order does job 2 need?
//
// TODO: change do_job() so it always locks the resource with the smaller
//       index first. (Unlock order does not matter for deadlock.)
//
// Test:  make test M=10 T=ex5_lock_ordering   (the unchanged code deadlocks and
//        the test's watchdog reports a timeout)
// Run:   make run  M=10 EX=ex5_lock_ordering
#include <pthread.h>
#include <sched.h>
#include <stdio.h>

#define NRES 3
static pthread_mutex_t res_lock[NRES] = {PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER,
                                         PTHREAD_MUTEX_INITIALIZER};
long uses[NRES];            // how often each resource was used (protected by its lock)

static const int needs[3][2] = {{0, 1}, {1, 2}, {2, 0}};   // A=0, B=1, C=2

// Runs job `j` once: takes both resources it needs, uses them, releases them.
void do_job(int j) {
    int first = needs[j][0], second = needs[j][1];
    if (first > second) {                // global order: lower index first
        int t = first; first = second; second = t;
    }
    pthread_mutex_lock(&res_lock[first]);
    sched_yield();                       // makes the bad timing much more likely
    pthread_mutex_lock(&res_lock[second]);
    uses[first]++;
    uses[second]++;
    pthread_mutex_unlock(&res_lock[second]);
    pthread_mutex_unlock(&res_lock[first]);
}

#ifndef TESTING
#include <unistd.h>
#define ROUNDS 100000
static volatile long progress[3];
static void *worker(void *arg) {
    int j = (int)(long)arg;
    for (int k = 0; k < ROUNDS; k++) { do_job(j); progress[j] = k + 1; }
    return NULL;
}
int main(void) {
    pthread_t t[3];
    for (long j = 0; j < 3; j++) pthread_create(&t[j], NULL, worker, (void *)j);
    long last = -1;
    for (;;) {
        usleep(500000);
        long now = progress[0] + progress[1] + progress[2];
        if (now == 3L * ROUNDS) break;
        if (now == last) {
            printf("DEADLOCK: no progress for half a second after %ld jobs.\n"
                   "Each job holds its first lock and waits for its second.\n", now);
            return 1;
        }
        last = now;
    }
    for (int j = 0; j < 3; j++) pthread_join(t[j], NULL);
    printf("all %d jobs finished: uses A=%ld B=%ld C=%ld (each should be %d)\n", 3 * ROUNDS,
           uses[0], uses[1], uses[2], 2 * ROUNDS);
    return 0;
}
#endif
