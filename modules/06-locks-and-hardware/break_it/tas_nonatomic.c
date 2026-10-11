// break_it/tas_nonatomic.c - a test-and-set lock whose test-and-set is NOT atomic.
//
// Run:   make run M=06 EX=tas_nonatomic                 (read, then always write: two steps)
//        make run M=06 EX=tas_nonatomic ARGS=checkfirst (read, then write only if it was free)
//        make run M=06 EX=tas_nonatomic ARGS=atomic     (the real one-step instruction)
//
// A test-and-set lock only works because the hardware does "read the old
// value" and "write true" as ONE step. Here we fake test-and-set with two
// separate steps. Two different things can then go wrong:
//
//  1. Two threads both read "free" before either writes "taken". Both walk
//     into the critical section. The counter is protected by the lock, so
//     every lost increment is a moment when two threads were inside together.
//     (Usually seen with ARGS=checkfirst.)
//  2. A waiting thread reads "taken", the holder unlocks (writes false), and
//     THEN the waiter writes its "taken". Now the lock says "taken" but
//     nobody holds it. Every thread spins forever: nobody ever gets in again.
//     (The explorer calls this a livelock: threads keep running, nobody enters.)
//     A waiter in the default version writes on every try, so this happens fast.
//
// Exam link: "what happens if TestAndSet is not atomic?" The model
// tools/interleave/programs/tas_nonatomic.il shows the exact bad orders.
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "atomics.h"

#define THREADS 4
#define ITERS 200000

static shared_bool held;
static long counter;              // protected by the lock (if the lock worked!)
static shared_long done[THREADS]; // how many rounds each thread has finished
static int mode;                  // 0 = two-step, 1 = check first, 2 = atomic

// The fake: read, then write. Another thread can run in between.
static bool fake_test_and_set(void) {
    bool old = LOAD(held);      // step 1: read
    STORE(held, true);          // step 2: write (another core may act in between)
    return old;
}

// A second fake: write only if the lock looked free. A waiter never writes,
// so problem 2 cannot happen, but problem 1 still can.
static bool check_then_set(void) {
    bool old = LOAD(held);
    if (!old) STORE(held, true);
    return old;
}

static void lock(void) {
    if (mode == 2)
        while (TEST_AND_SET(held)) cpu_relax();
    else if (mode == 1)
        while (check_then_set()) cpu_relax();
    else
        while (fake_test_and_set()) cpu_relax();
}

static void unlock(void) { STORE(held, false); }

static void *worker(void *arg) {
    int id = (int)(long)arg;
    for (int k = 0; k < ITERS; k++) {
        lock();
        counter++;
        unlock();
        STORE(done[id], k + 1);
    }
    return NULL;
}

static long total_done(void) {
    long s = 0;
    for (int i = 0; i < THREADS; i++) s += LOAD(done[i]);
    return s;
}

int main(int argc, char **argv) {
    mode = argc < 2 ? 0 : strcmp(argv[1], "atomic") == 0 ? 2 : strcmp(argv[1], "checkfirst") == 0 ? 1 : 0;
    const char *name = mode == 2 ? "atomic" : mode == 1 ? "check-first (fake)" : "two-step (fake)";
    pthread_t t[THREADS];
    for (long i = 0; i < THREADS; i++) pthread_create(&t[i], NULL, worker, (void *)i);

    // Watch for progress. If nothing happens for half a second, report it.
    long expected = (long)THREADS * ITERS, last = -1;
    for (;;) {
        usleep(500000);
        long now = total_done();
        if (now == expected) break;
        if (now == last) {
            // Every thread is spinning in lock(), so nobody touches counter now.
            printf("%s test-and-set: STUCK after %ld of %ld rounds.\n"
                   "=> problem 2: the lock says \"taken\" (held = %s) but no thread is inside.\n"
                   "   A waiter read \"taken\", then the holder unlocked, then the waiter\n"
                   "   wrote \"taken\" again. Now every thread spins forever.\n",
                   name, now, expected, LOAD(held) ? "true" : "false");
            if (now != counter)
                printf("=> problem 1 too: before that, %ld of the %ld finished rounds lost\n"
                       "   their increment, because two threads were inside together.\n",
                       now - counter, now);
            printf("   Try: ARGS=checkfirst (problem 1 only) or ARGS=atomic\n");
            return 1;                 // the stuck threads end with the process
        }
        last = now;
    }
    for (int i = 0; i < THREADS; i++) pthread_join(t[i], NULL);
    printf("%s test-and-set: counter = %ld, expected %ld, lost %ld increments\n",
           name, counter, expected, expected - counter);
    if (counter != expected)
        printf("=> two threads were in the critical section together at least %ld times.\n"
               "   Both read held == false, then both wrote true, and both went in.\n"
               "   Try: ARGS=atomic\n", expected - counter);
    else
        printf("=> no lost increments this run.%s\n",
               mode == 2 ? " The hardware does the read and the write as one step."
                          : " Run again: it depends on timing.");
    return 0;
}
