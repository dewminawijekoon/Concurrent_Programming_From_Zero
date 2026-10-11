// Helpers shared by the module 09 tests.
#ifndef CLASSIC_TEST_UTIL_H
#define CLASSIC_TEST_UTIL_H

#include <stdatomic.h>
#include <time.h>

#include "harness.h"
#include "trace.h"

static inline void sleep_ms(int ms) {
    struct timespec ts = {ms / 1000, (long)(ms % 1000) * 1000000L};
    nanosleep(&ts, NULL);
}

static inline void sleep_us(long us) {
    struct timespec ts = {us / 1000000L, (us % 1000000L) * 1000L};
    nanosleep(&ts, NULL);
}

// Wait until *flag becomes non-zero, for at most `ms` milliseconds.
static inline int wait_for_flag(atomic_int *flag, int ms) {
    for (int waited = 0; waited < ms; waited++) {
        if (atomic_load(flag)) return 1;
        sleep_ms(1);
    }
    return atomic_load(flag) != 0;
}

static inline void atomic_max(atomic_int *max, int now) {
    int old = atomic_load(max);
    while (now > old && !atomic_compare_exchange_weak(max, &old, now)) {
    }
}

// ---------------------------------------------------------------- progress watchdog
// A deadlock means "nothing happens any more". The test bumps `progress`
// whenever something useful happens (a meal, a crossing, ...). If it stays
// the same for `stall_seconds`, we print what we know and fail.
static atomic_long progress;
static atomic_int pw_running;
static int pw_stall_seconds;
static const char *pw_what;
static void (*pw_report)(void);

static void *pw_thread(void *arg) {
    (void)arg;
    long last = -1;
    int still = 0;
    while (atomic_load(&pw_running)) {
        sleep_ms(100);
        long now = atomic_load(&progress);
        if (now == last) {
            still++;
        } else {
            still = 0;
            last = now;
        }
        if (still >= pw_stall_seconds * 10 && atomic_load(&pw_running)) {
            printf("    FAIL: nothing happened for %d seconds: %s\n", pw_stall_seconds, pw_what);
            if (pw_report) pw_report();
            fflush(stdout);
            _exit(1);
        }
    }
    return NULL;
}

static pthread_t pw_tid;
static inline void progress_watch_start(int stall_seconds, const char *what, void (*report)(void)) {
    atomic_store(&progress, 0);
    pw_stall_seconds = stall_seconds;
    pw_what = what;
    pw_report = report;
    atomic_store(&pw_running, 1);
    pthread_create(&pw_tid, NULL, pw_thread, NULL);
}
static inline void progress_watch_stop(void) {
    atomic_store(&pw_running, 0);
    pthread_join(pw_tid, NULL);
}

#endif
