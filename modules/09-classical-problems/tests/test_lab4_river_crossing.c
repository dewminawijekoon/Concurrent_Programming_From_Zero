// TIMEOUT: 60
// Tests for lab track 4 (Lab 04 Ex 4): every boat is a valid group of 4.
#include <string.h>

#include "classic_test_util.h"

void river_init(void);
void river_destroy(void);
void hacker_arrives(int id);
void serf_arrives(int id);

void river_board(int kind, int id) {
    trace_event(kind * 1000 + id, kind == 0 ? "board_H" : "board_S", kind);
}
void river_row(int kind, int id) {
    trace_event(kind * 1000 + id, "row", kind);
    atomic_fetch_add(&progress, 1);
}

static int n_h, n_s, max_delay_us;
static void person(int k, void *ctx) {
    (void)ctx;
    if (max_delay_us) sleep_us(harness_rand() % (unsigned)max_delay_us);
    if (k < n_h) hacker_arrives(k);
    else serf_arrives(k - n_h);
}

static void report(void) {
    printf("          (with these numbers everyone can cross, so somebody is waiting for nothing)\n");
    trace_dump_tail(12);
}

// Returns 1 if the run followed every rule.
static int crossing_day(int h, int s, int delay_us) {
    n_h = h;
    n_s = s;
    max_delay_us = delay_us;
    trace_reset();
    river_init();
    progress_watch_start(4, "people are still waiting on the bank (deadlock?)", report);
    harness_parallel(h + s, person, NULL);
    progress_watch_stop();
    river_destroy();

    static int boarded_h[1000], boarded_s[1000];
    memset(boarded_h, 0, sizeof boarded_h);
    memset(boarded_s, 0, sizeof boarded_s);
    int in_boat = 0, bh = 0, bs = 0, rows = 0, bad = 0;
    for (long i = 0; i < trace_len && !bad; i++) {
        trace_entry *e = &trace_log[i];
        if (strcmp(e->action, "row") == 0) {
            int valid = in_boat == 4 && (bh == 4 || bs == 4 || (bh == 2 && bs == 2));
            if (!valid) {
                printf("    a boat sailed with %d hackers and %d serfs (log entry %ld)\n", bh, bs, i);
                bad = 1;
            }
            rows++;
            in_boat = bh = bs = 0;
        } else {
            in_boat++;
            int who = e->thread % 1000;
            if (e->value == 0) {
                bh++;
                boarded_h[who]++;
            } else {
                bs++;
                boarded_s[who]++;
            }
            if (in_boat > 4) {
                printf("    a 5th person boarded before the boat had left (log entry %ld)\n", i);
                bad = 1;
            }
        }
    }
    if (!bad && in_boat != 0) {
        printf("    %d people boarded a boat that never sailed\n", in_boat);
        bad = 1;
    }
    for (int i = 0; i < h && !bad; i++)
        if (boarded_h[i] != 1) {
            printf("    hacker %d boarded %d times (expected once)\n", i, boarded_h[i]);
            bad = 1;
        }
    for (int i = 0; i < s && !bad; i++)
        if (boarded_s[i] != 1) {
            printf("    serf %d boarded %d times (expected once)\n", i, boarded_s[i]);
            bad = 1;
        }
    if (!bad && rows != (h + s) / 4) {
        printf("    %d boats sailed, expected %d\n", rows, (h + s) / 4);
        bad = 1;
    }
    CHECK(!bad, "H=%d S=%d: the crossing broke a rule (see above)", h, s);
    if (bad) trace_dump_tail(12);
    return !bad;
}

static void test_lab_cases(void) {   // Lab Q4.3
    int cases[][2] = {{4, 4}, {8, 0}, {0, 8}, {6, 2}, {2, 6}};
    for (int c = 0; c < 5; c++)
        for (int rep = 0; rep < 20; rep++)
            if (!crossing_day(cases[c][0], cases[c][1], 2000)) return;
}

static void test_twelve_and_twelve(void) {
    long reps = harness_iters(30);
    for (long r = 0; r < reps; r++)
        if (!crossing_day(12, 12, r % 2 ? 3000 : 0)) return;
    printf("  (%ld days with 12 hackers and 12 serfs: every boat valid - observed, not proved)\n", reps);
}

static void test_big_crowd(void) {
    for (int r = 0; r < 5; r++)
        if (!crossing_day(40, 24, 0)) return;
}

int main(void) {
    harness_watchdog(50);
    harness_hint("The thread that completes a group (4 of a kind, or 2+2) is the captain: it posts the "
                 "queue semaphores once per seat and KEEPS the mutex; the others release the mutex and "
                 "wait on their queue. Everyone boards; the captain waits for 3 'boarded' posts, rows, "
                 "and only then releases the mutex.");
    RUN_TEST(test_lab_cases);
    RUN_TEST(test_twelve_and_twelve);
    RUN_TEST(test_big_crowd);
    return harness_report();
}
