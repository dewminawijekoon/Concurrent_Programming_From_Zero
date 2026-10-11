// Lab track 1 - Single-lane bridge, one direction at a time, at most 3 cars   [Lab 04, Ex 1]
//
// ANSWER. Three building blocks from module 07:
//   - one LIGHTSWITCH per direction on the semaphore `bridge_free`: the
//     first car of a direction locks the bridge for its whole direction,
//     the last car of that direction to leave unlocks it;
//   - a MULTIPLEX (semaphore starting at 3) for the capacity.
// Order: lightswitch first, then multiplex. A car only takes a capacity
// ticket once its direction owns the bridge.
//
// This solution can STARVE one direction (Lab Q1.2): while cars keep
// coming from the north, the north lightswitch never reaches 0. Lab track 2
// fixes that with a turnstile.
//
// Test:  make test M=09 T=lab1_bridge
// Run:   make run  M=09 EX=lab1_bridge ANS=1
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

static sem_t bridge_free;        // 1 = no direction owns the bridge
static sem_t capacity;           // free places on the bridge
static lightswitch_t dir_switch[2];

void bridge_init(int max_cars) {
    sem_init(&bridge_free, 0, 1);
    sem_init(&capacity, 0, (unsigned)max_cars);
    for (int d = 0; d < 2; d++) {
        dir_switch[d].counter = 0;
        sem_init(&dir_switch[d].mutex, 0, 1);
    }
}

void bridge_destroy(void) {
    sem_destroy(&bridge_free);
    sem_destroy(&capacity);
    for (int d = 0; d < 2; d++) sem_destroy(&dir_switch[d].mutex);
}

// dir: 0 = N->S, 1 = S->N
void bridge_enter(int dir) {
    ls_lock(&dir_switch[dir], &bridge_free);   // my direction owns the bridge
    sem_wait(&capacity);                       // and there is room for me
}

void bridge_exit(int dir) {
    sem_post(&capacity);
    ls_unlock(&dir_switch[dir], &bridge_free);
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
