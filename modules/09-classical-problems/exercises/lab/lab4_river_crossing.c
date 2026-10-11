// Lab track 4 - River crossing: boats of 4, only valid groups   [Lab 04, Ex 4]
//
// Hackers and serfs wait on a river bank. A boat takes exactly 4 people,
// and only these groups may sail: 4 hackers, 4 serfs, or 2 hackers + 2 serfs
// (3 + 1 is NOT allowed). The thread that completes a valid group is the
// CAPTAIN: once all 4 are on board, it rows.
//
// Two hooks are given (the test records them, main() prints them):
//   river_board(kind, id)  every thread calls this once, when it gets on its boat
//   river_row(kind, id)    the captain calls this once, after all 4 have boarded
// The test checks that the log is always: 4 boardings (a valid group),
// then 1 row, then the next 4 boardings, and so on.
//
// PREDICT FIRST: a thread arrives and sees 3 hackers and 1 serf waiting.
//                It is a serf. Which group sails? And if it is a hacker?
//                How do you stop a 5th thread from boarding while a group
//                is still getting on? (Lab Q4.1)
//
// Hints (Lab 04): two counters (hackers and serfs waiting) protected by a
// mutex semaphore; one queue semaphore per kind (start at 0) that the
// captain posts once for each seat; a way for the captain to wait until
// the other 3 have boarded. Keep the mutex until the boat has left.
//
// Test:  make test M=09 T=lab4_river_crossing
// Run:   make run  M=09 EX=lab4_river_crossing ARGS="12 12"
//        try ARGS="4 4", "8 0", "6 2" and "5 3" (Lab Q4.3)
#include <pthread.h>
#include <semaphore.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define HACKER 0
#define SERF 1

// Provided by the test (or by main below):
void river_board(int kind, int id);   // every thread calls it once, when it gets on the boat
void river_row(int kind, int id);     // the captain calls it once, after all 4 have boarded

// TODO: declare your semaphores and counters

void river_init(void) {
    // TODO
}

void river_destroy(void) {
    // TODO
}

void hacker_arrives(int id) {
    (void)id;   // TODO: wait until you are part of a valid group, board, and row if you are the captain
}

void serf_arrives(int id) {
    (void)id;   // TODO
}

#ifndef TESTING
static atomic_int crossings, invalid;
static int boat_h, boat_s, boat_ids[4], boat_kind[4], boat_n;
static pthread_mutex_t log_mu = PTHREAD_MUTEX_INITIALIZER;

void river_board(int kind, int id) {
    pthread_mutex_lock(&log_mu);
    printf("%s %2d: boarding\n", kind == HACKER ? "Hacker" : "Serf  ", id);
    if (boat_n < 4) {
        boat_ids[boat_n] = id;
        boat_kind[boat_n++] = kind;
    }
    if (kind == HACKER) boat_h++;
    else boat_s++;
    pthread_mutex_unlock(&log_mu);
}
void river_row(int kind, int id) {
    pthread_mutex_lock(&log_mu);
    int ok = boat_h + boat_s == 4 && (boat_h == 4 || boat_s == 4 || boat_h == 2);
    printf("%s %2d: rowing as captain! (group: %dH + %dS%s)\n", kind == HACKER ? "Hacker" : "Serf  ", id,
           boat_h, boat_s, ok ? "" : " - INVALID");
    printf("Crossing complete: [");
    for (int i = 0; i < boat_n; i++)
        printf("%s%c%d", i ? ", " : "", boat_kind[i] == HACKER ? 'H' : 'S', boat_ids[i]);
    printf("]\n");
    if (!ok) atomic_fetch_add(&invalid, 1);
    atomic_fetch_add(&crossings, 1);
    boat_h = boat_s = boat_n = 0;
    pthread_mutex_unlock(&log_mu);
}

static void *person(void *arg) {
    long v = (long)arg;
    int kind = (int)(v & 1), id = (int)(v >> 1);
    unsigned seed = 7u + (unsigned)v;
    usleep(rand_r(&seed) % 50000);
    printf("%s %2d: arrived at bank\n", kind == HACKER ? "Hacker" : "Serf  ", id);
    if (kind == HACKER) hacker_arrives(id);
    else serf_arrives(id);
    return NULL;
}

int main(int argc, char **argv) {
    int h = argc > 1 ? atoi(argv[1]) : 12, s = argc > 2 ? atoi(argv[2]) : 12;
    if ((h + s) % 4 != 0 || h % 2 != 0 || s % 2 != 0)
        printf("Warning: with H=%d and S=%d not everyone can cross; the program will hang "
               "at the end (press Ctrl+C). See the quiz.\n", h, s);
    pthread_t *t = malloc(sizeof *t * (size_t)(h + s));
    river_init();
    int k = 0;
    for (long i = 0; i < h; i++) pthread_create(&t[k++], NULL, person, (void *)(i << 1 | HACKER));
    for (long i = 0; i < s; i++) pthread_create(&t[k++], NULL, person, (void *)(i << 1 | SERF));
    for (int i = 0; i < k; i++) pthread_join(t[i], NULL);
    river_destroy();
    printf("Total crossings: %d, invalid groups: %d\n", atomic_load(&crossings), atomic_load(&invalid));
    free(t);
    return 0;
}
#endif
