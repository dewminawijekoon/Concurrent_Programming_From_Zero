// Lab track 1 - Single-lane bridge, one direction at a time, at most 3 cars   [Lab 04, Ex 1]
//
// Cars cross a narrow bridge from the north (dir 0, "N->S") or from the
// south (dir 1, "S->N"). Rules:
//   (a) only one direction at a time on the bridge;
//   (b) at most `max_cars` cars on the bridge (3 in the lab).
// Cars going the SAME way may be on the bridge together.
//
// PREDICT FIRST: which two patterns from module 07 match rules (a) and (b)?
//                How many semaphores will you need in total? (Lab Q1.1)
//                Can one direction starve in your design? (Lab Q1.2)
//
// TODO: bridge_init / bridge_enter / bridge_exit / bridge_destroy with
//       semaphores. The lightswitch from module 07 is given below; use it if
//       it fits. The main() at the bottom is the lab's program (20 cars,
//       messages, self-validation and summary); it is finished.
//
// Test:  make test M=09 T=lab1_bridge
// Run:   make run  M=09 EX=lab1_bridge
#include <pthread.h>
#include <semaphore.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

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

void bridge_init(int max_cars) {
    (void)max_cars;   // TODO
    (void)ls_lock;    // (these two lines only silence 'unused' warnings;
    (void)ls_unlock;  //  delete them if you use the lightswitch)
}

void bridge_destroy(void) {
    // TODO
}

// dir: 0 = N->S, 1 = S->N
void bridge_enter(int dir) {
    (void)dir;   // TODO
}

void bridge_exit(int dir) {
    (void)dir;   // TODO
}

#ifndef TESTING
#define MAX_ON_BRIDGE 3
#define CARS_PER_DIR 10

typedef struct {
    int id, direction;
    unsigned seed;
} car_t;

static atomic_int on_bridge[2];
static atomic_int violations, passed;

static void check(void) {   // the lab's self-validation
    int n = atomic_load(&on_bridge[0]), s = atomic_load(&on_bridge[1]);
    if ((n > 0 && s > 0) || n > MAX_ON_BRIDGE || s > MAX_ON_BRIDGE) {
        atomic_fetch_add(&violations, 1);
        printf("  VIOLATION: N->S = %d, S->N = %d\n", n, s);
    }
}

static void *car_thread(void *arg) {
    car_t *car = arg;
    const char *dir = car->direction == 0 ? "N->S" : "S->N";
    usleep(rand_r(&car->seed) % 100000);   // random arrival
    bridge_enter(car->direction);
    atomic_fetch_add(&on_bridge[car->direction], 1);
    check();
    printf("Car %2d [%s]: entering bridge (on bridge: %d)\n", car->id, dir,
           atomic_load(&on_bridge[car->direction]));
    usleep(rand_r(&car->seed) % 50000);    // crossing time
    printf("Car %2d [%s]: exiting bridge\n", car->id, dir);
    atomic_fetch_sub(&on_bridge[car->direction], 1);
    check();
    atomic_fetch_add(&passed, 1);
    bridge_exit(car->direction);
    return NULL;
}

int main(void) {
    pthread_t t[2 * CARS_PER_DIR];
    car_t cars[2 * CARS_PER_DIR];
    bridge_init(MAX_ON_BRIDGE);
    for (int i = 0; i < 2 * CARS_PER_DIR; i++) {
        cars[i] = (car_t){i, i % 2, 42u + (unsigned)i};
        pthread_create(&t[i], NULL, car_thread, &cars[i]);
    }
    for (int i = 0; i < 2 * CARS_PER_DIR; i++) pthread_join(t[i], NULL);
    bridge_destroy();
    printf("Summary: %d cars passed, %d violations\n", atomic_load(&passed), atomic_load(&violations));
    return 0;
}
#endif
