// Lab track 2 - Construction zone: capacity N, one direction, and FAIR   [Lab 04, Ex 2; L08 slide 35]
//
// Like lab track 1, with two changes:
//   - the capacity N comes from the command line;
//   - it must be FAIR: a steady stream of trucks from one side must not
//     keep the other side waiting for ever. Once a truck is waiting for
//     the other direction to clear, trucks that arrive LATER (from either
//     side) must not overtake it.
// Types: 0 = supply truck (to the site), 1 = return truck (from the site).
//
// PREDICT FIRST: in lab track 1, a supply truck is inside and a return truck
//                waits. Another supply truck arrives. Does it go first?
//                Which pattern from exercise 1 (readers-writers) stops that?
//
// TODO: zone_init / zone_enter / zone_exit / zone_destroy. The lightswitch
//       is given. main() is the lab's program (finished): it prints the
//       timestamped log, counts violations, and with ARGS="table" prints
//       the timing table for N = 1, 3, 5 (Lab Q2.2).
//
// Test:  make test M=09 T=lab2_construction_zone
// Run:   make run  M=09 EX=lab2_construction_zone ARGS="3"
//        make run  M=09 EX=lab2_construction_zone ARGS="table"
#include <pthread.h>
#include <semaphore.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "bench.h"

typedef struct {
    int counter;
    sem_t mutex;
} lightswitch_t;

static void ls_lock(lightswitch_t *ls, sem_t *s) {
    sem_wait(&ls->mutex);
    if (++ls->counter == 1) sem_wait(s);
    sem_post(&ls->mutex);
}
static void ls_unlock(lightswitch_t *ls, sem_t *s) {
    sem_wait(&ls->mutex);
    if (--ls->counter == 0) sem_post(s);
    sem_post(&ls->mutex);
}

// TODO: declare your semaphores and shared state here

void zone_init(int n) {
    (void)n;          // TODO
    (void)ls_lock;    // (these two lines only silence 'unused' warnings;
    (void)ls_unlock;  //  delete them if you use the lightswitch)
}

void zone_destroy(void) {
    // TODO
}

// type: 0 = supply truck (to the site), 1 = return truck (from the site)
void zone_enter(int type) {
    (void)type;   // TODO
}

void zone_exit(int type) {
    (void)type;   // TODO
}

#ifndef TESTING
#define SUPPLY 15
#define RETURN 15
static double t0;
static int max_n;
static atomic_int inside[2];
static atomic_int dir_violations, cap_violations, passed;
static int quiet;

static void *truck(void *arg) {
    int id = (int)(long)arg;
    int type = id < SUPPLY ? 0 : 1;
    unsigned seed = 1234u + (unsigned)id;
    usleep(rand_r(&seed) % 100000);   // random arrival
    zone_enter(type);
    int me = atomic_fetch_add(&inside[type], 1) + 1;
    int other = atomic_load(&inside[1 - type]);
    if (other > 0) atomic_fetch_add(&dir_violations, 1);
    if (me > max_n) atomic_fetch_add(&cap_violations, 1);
    if (!quiet)
        printf("[%7.3fs] %s %2d: entered (supply=%d, return=%d)\n", bench_now() - t0,
               type == 0 ? "Supply" : "Return", id, atomic_load(&inside[0]), atomic_load(&inside[1]));
    usleep(10000 + rand_r(&seed) % 20000);   // time inside the zone
    atomic_fetch_sub(&inside[type], 1);
    if (!quiet)
        printf("[%7.3fs] %s %2d: exited  (supply=%d, return=%d)\n", bench_now() - t0,
               type == 0 ? "Supply" : "Return", id, atomic_load(&inside[0]), atomic_load(&inside[1]));
    atomic_fetch_add(&passed, 1);
    zone_exit(type);
    return NULL;
}

static double run_once(int n) {
    pthread_t t[SUPPLY + RETURN];
    max_n = n;
    atomic_store(&passed, 0);
    zone_init(n);
    t0 = bench_now();
    for (long i = 0; i < SUPPLY + RETURN; i++) pthread_create(&t[i], NULL, truck, (void *)i);
    for (int i = 0; i < SUPPLY + RETURN; i++) pthread_join(t[i], NULL);
    double secs = bench_now() - t0;
    zone_destroy();
    return secs;
}

int main(int argc, char **argv) {
    if (argc > 1 && strcmp(argv[1], "table") == 0) {   // Lab Q2.2
        quiet = 1;
        printf("| N | total time (s) | vehicles | direction violations | capacity violations |\n");
        printf("|--:|---------------:|---------:|---------------------:|--------------------:|\n");
        int ns[] = {1, 3, 5};
        for (int k = 0; k < 3; k++) {
            double secs = run_once(ns[k]);
            printf("| %d | %14.3f | %8d | %20d | %19d |\n", ns[k], secs, atomic_load(&passed),
                   atomic_load(&dir_violations), atomic_load(&cap_violations));
        }
        return 0;
    }
    int n = argc > 1 ? atoi(argv[1]) : 3;
    if (n < 1) n = 1;
    double secs = run_once(n);
    printf("Total: %d vehicles passed in %.3f s; direction violations: %d; capacity violations: %d\n",
           atomic_load(&passed), secs, atomic_load(&dir_violations), atomic_load(&cap_violations));
    return 0;
}
#endif
