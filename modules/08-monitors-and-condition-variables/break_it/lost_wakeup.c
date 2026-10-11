// break_it/lost_wakeup.c - two ways to lose a wake-up, and the fix.
//
// Run:   make run M=08 EX=lost_wakeup                (all three versions)
//        make run M=08 EX=lost_wakeup ARGS=nostate   (only one of: nostate, nolock, correct)
//
// A condition variable has NO memory. pthread_cond_signal() wakes a thread
// that is waiting right now; if nobody is waiting, the signal is simply
// thrown away. A thread that starts waiting a moment later sleeps forever.
// That is a "lost wake-up". (Semaphores are different: up() is remembered
// in the counter.)
//
// The waiter here waits at most 1 second (pthread_cond_timedwait), so the
// demo can tell you "this wake-up was lost" instead of hanging for ever.
//
//   nostate: no flag at all. The signal comes before the wait: lost.
//   nolock:  there is a flag, and the waiter checks it while holding the
//            mutex, but the signaller sets the flag and signals WITHOUT
//            taking the mutex. It slips in between the waiter's check and
//            its wait: lost. (We widen that gap with a short sleep so it
//            happens every time.)
//   correct: flag + the signaller takes the mutex. It cannot run in the gap,
//            because the waiter holds the mutex from its check until
//            pthread_cond_wait() releases it (atomically, while it starts
//            sleeping). Nothing is lost.
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

static pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t c = PTHREAD_COND_INITIALIZER;
static int ready;                 // the flag ("the thing I wait for has happened")
static const char *mode;

// Wait at most 1 second. Returns 0 if woken, ETIMEDOUT if nobody woke us.
static int wait_one_second(void) {
    struct timespec t;
    clock_gettime(CLOCK_REALTIME, &t);
    t.tv_sec += 1;
    return pthread_cond_timedwait(&c, &m, &t);
}

static void *signaller(void *arg) {
    (void)arg;
    usleep(50000);                                // let the waiter go first (not in nostate)
    if (strcmp(mode, "correct") == 0) {
        pthread_mutex_lock(&m);
        ready = 1;
        pthread_cond_signal(&c);
        pthread_mutex_unlock(&m);
    } else {                                      // nolock: no mutex - the bug
        ready = 1;
        pthread_cond_signal(&c);
    }
    return NULL;
}

static void run(const char *which) {
    mode = which;
    ready = 0;
    int r = 0;
    if (strcmp(mode, "nostate") == 0) {
        pthread_cond_signal(&c);                  // "it has happened!" - nobody is listening
        pthread_mutex_lock(&m);
        r = wait_one_second();                    // waits for a signal that already went by
        pthread_mutex_unlock(&m);
    } else {
        pthread_t t;
        pthread_create(&t, NULL, signaller, NULL);
        pthread_mutex_lock(&m);
        if (!ready) {                             // check the flag...
            usleep(100000);                       // (the gap, made wide on purpose)
            r = wait_one_second();                // ...then sleep
        }
        pthread_mutex_unlock(&m);
        pthread_join(t, NULL);
    }
    printf("%-8s: %s\n", which, r == ETIMEDOUT ? "LOST wake-up (would have slept forever)" : "woken up correctly");
}

int main(int argc, char **argv) {
    if (argc > 1) { run(argv[1]); return 0; }
    run("nostate");
    run("nolock");
    run("correct");
    printf("\nRules: (1) keep the state in a variable and wait WHILE it is not true;\n"
           "       (2) change that variable and signal while HOLDING the mutex.\n");
    return 0;
}
