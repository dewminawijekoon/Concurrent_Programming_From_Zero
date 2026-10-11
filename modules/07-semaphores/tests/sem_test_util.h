// Small helpers shared by the module 07 tests.
#ifndef SEM_TEST_UTIL_H
#define SEM_TEST_UTIL_H

#include <stdatomic.h>
#include <time.h>

#include "harness.h"

static inline void sleep_ms(int ms) {
    struct timespec ts = {ms / 1000, (long)(ms % 1000) * 1000000L};
    nanosleep(&ts, NULL);
}

// Wait until *flag becomes non-zero, for at most `ms` milliseconds.
// Returns 1 if the flag was set in time, 0 if we gave up.
static inline int wait_for_flag(atomic_int *flag, int ms) {
    for (int waited = 0; waited < ms; waited++) {
        if (atomic_load(flag)) return 1;
        sleep_ms(1);
    }
    return atomic_load(flag) != 0;
}

// Raise `max` to `now` if `now` is bigger (keeps the largest value seen).
static inline void atomic_max(atomic_int *max, int now) {
    int old = atomic_load(max);
    while (now > old && !atomic_compare_exchange_weak(max, &old, now)) {
    }
}

#endif
