// Helpers for the module 08 tests.
//
// Many tests here ask "does this call block?" or "does this call finish?".
// If we simply called the function and it never returned, the whole test
// would hang. So we run the call in a helper thread (a "job") and then look
// at it from the outside:
//
//   cvt_job *j = cvt_start(fn, arg);      // run fn(arg) in a new thread
//   sleep_ms(100);
//   CHECK(!cvt_done(j), "it should still be waiting");   // still blocked?
//   ... do something that should release it ...
//   CHECK(cvt_wait(j, 5.0), "it never finished");          // finished in time?
//
// Timing never decides PASS for a correct solution: a correct blocked call
// stays blocked however long we sleep, and a correct call that should finish
// gets 5 whole seconds. Short sleeps only make it more likely that a WRONG
// solution is caught.
#ifndef CV_HELPERS_H
#define CV_HELPERS_H

#include <pthread.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <time.h>

static inline void sleep_ms(long ms) {
    struct timespec ts = {ms / 1000, (ms % 1000) * 1000000L};
    nanosleep(&ts, NULL);
}

typedef struct {
    void (*fn)(void *);
    void *arg;
    atomic_int done;
} cvt_job;

static void *cvt_main(void *p) {
    cvt_job *j = p;
    j->fn(j->arg);
    atomic_store(&j->done, 1);
    return NULL;
}

// Run fn(arg) in a new (detached) thread. The job is never freed: a job that
// is stuck forever must not point to freed memory.
static inline cvt_job *cvt_start(void (*fn)(void *), void *arg) {
    cvt_job *j = calloc(1, sizeof *j);
    j->fn = fn;
    j->arg = arg;
    atomic_init(&j->done, 0);
    pthread_t t;
    pthread_attr_t a;
    pthread_attr_init(&a);
    pthread_attr_setdetachstate(&a, PTHREAD_CREATE_DETACHED);
    if (pthread_create(&t, &a, cvt_main, j) != 0) {
        perror("pthread_create");
        exit(1);
    }
    pthread_attr_destroy(&a);
    return j;
}

static inline int cvt_done(cvt_job *j) { return atomic_load(&j->done); }

// Wait up to `secs` seconds for the job to finish. Returns 1 if it finished.
static inline int cvt_wait(cvt_job *j, double secs) {
    for (long waited = 0; waited < (long)(secs * 1000); waited++) {
        if (cvt_done(j)) return 1;
        sleep_ms(1);
    }
    return cvt_done(j);
}

#endif
