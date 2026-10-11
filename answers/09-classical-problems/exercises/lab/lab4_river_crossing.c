// Lab track 4 - River crossing: boats of 4, only valid groups   [Lab 04, Ex 4]
//
// ANSWER. Semaphores:
//   mutex     = 1  protects the two waiting counters. The CAPTAIN keeps it
//                  until the boat has left, so no new group can form meanwhile.
//   hacker_q  = 0  hackers wait here until a captain picks them for a boat
//   serf_q    = 0  the same for serfs
//   boarded   = 0  each of the 3 passengers posts it after boarding; the
//                  captain waits for 3 posts before rowing
// The thread that COMPLETES a valid group becomes the captain. It posts the
// queue semaphores once per seat (its own seat included), so exactly the 4
// chosen threads get through. Nobody else can be waiting on a queue at that
// moment: every other arrival is stuck on `mutex`, which the captain holds.
//
// Test:  make test M=09 T=lab4_river_crossing
// Run:   make run  M=09 EX=lab4_river_crossing ANS=1 ARGS="12 12"
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

static sem_t mutex, hacker_q, serf_q, boarded;
static int hackers, serfs;   // waiting on the bank, not yet in a group

void river_init(void) {
    hackers = serfs = 0;
    sem_init(&mutex, 0, 1);
    sem_init(&hacker_q, 0, 0);
    sem_init(&serf_q, 0, 0);
    sem_init(&boarded, 0, 0);
}

void river_destroy(void) {
    sem_destroy(&mutex);
    sem_destroy(&hacker_q);
    sem_destroy(&serf_q);
    sem_destroy(&boarded);
}

static void post_n(sem_t *s, int n) {
    for (int i = 0; i < n; i++) sem_post(s);
}

static void arrive(int kind, int id) {
    int captain = 0;
    sem_wait(&mutex);
    int *mine = kind == HACKER ? &hackers : &serfs;
    int *other = kind == HACKER ? &serfs : &hackers;
    sem_t *my_q = kind == HACKER ? &hacker_q : &serf_q;
    sem_t *other_q = kind == HACKER ? &serf_q : &hacker_q;
    (*mine)++;
    if (*mine == 4) {                          // 4 of my kind
        post_n(my_q, 4);
        *mine -= 4;
        captain = 1;
    } else if (*mine == 2 && *other >= 2) {    // 2 + 2
        post_n(my_q, 2);
        post_n(other_q, 2);
        *mine -= 2;
        *other -= 2;
        captain = 1;
    } else {
        sem_post(&mutex);                      // no group yet: let others arrive
    }
    sem_wait(my_q);                            // wait until picked for a boat
    river_board(kind, id);
    if (captain) {
        for (int i = 0; i < 3; i++) sem_wait(&boarded);   // everyone is on board
        river_row(kind, id);
        sem_post(&mutex);                      // the boat has left: next group may form
    } else {
        sem_post(&boarded);
    }
}

void hacker_arrives(int id) { arrive(HACKER, id); }
void serf_arrives(int id) { arrive(SERF, id); }

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
