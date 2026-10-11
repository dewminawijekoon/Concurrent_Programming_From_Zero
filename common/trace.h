/*
 * trace.h — a thread-safe event log for synchronization problems.
 *
 * Tests for problems such as the bridge, the barbershop or readers-writers
 * record what each thread does, then check the problem's rules (invariants)
 * against the log, e.g. "never more than N cars on the bridge" or
 * "never a reader and a writer at the same time".
 *
 *   trace_event(id, "enter", dir);   // record: thread id, action, value
 *   ...
 *   trace_dump_tail(20);             // print the last 20 events on failure
 *
 * Exercises may also call trace_event() to print a readable log when run
 * stand-alone (set TRACE_PRINT=1 in the environment to echo every event).
 */
#ifndef TRACE_H
#define TRACE_H

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef TRACE_CAPACITY
#define TRACE_CAPACITY 200000
#endif

typedef struct {
    long seq;           /* global order of the event */
    int thread;         /* who did it                */
    const char *action; /* what happened (static string) */
    long value;         /* optional detail           */
} trace_entry;

static trace_entry trace_log[TRACE_CAPACITY];
static long trace_len = 0;
static pthread_mutex_t trace_mu = PTHREAD_MUTEX_INITIALIZER;
static int trace_echo = -1;

/* Record one event. The mutex makes the log a single total order. */
static inline void trace_event(int thread, const char *action, long value) {
    pthread_mutex_lock(&trace_mu);
    if (trace_echo < 0) trace_echo = getenv("TRACE_PRINT") != NULL;
    long i = trace_len;
    if (i < TRACE_CAPACITY) {
        trace_log[i] = (trace_entry){i, thread, action, value};
        trace_len++;
    }
    if (trace_echo) printf("[%6ld] T%-3d %-12s %ld\n", i, thread, action, value);
    pthread_mutex_unlock(&trace_mu);
}

static inline void trace_reset(void) {
    pthread_mutex_lock(&trace_mu);
    trace_len = 0;
    pthread_mutex_unlock(&trace_mu);
}

static inline void trace_dump_tail(long n) {
    long start = trace_len > n ? trace_len - n : 0;
    printf("    last %ld events:\n", trace_len - start);
    for (long i = start; i < trace_len; i++)
        printf("      [%6ld] T%-3d %-12s %ld\n", trace_log[i].seq,
               trace_log[i].thread, trace_log[i].action, trace_log[i].value);
}

/* Count how many logged events have the given action. */
static inline long trace_count(const char *action) {
    long c = 0;
    for (long i = 0; i < trace_len; i++)
        if (strcmp(trace_log[i].action, action) == 0) c++;
    return c;
}

#endif /* TRACE_H */
