// Exercise 2 (core) - Dining philosophers without deadlock   [L08 (semaphores) slides 26-28; L03 slide 11]
//
// n philosophers sit around a table with one fork between each pair.
// Fork i lies between philosopher i and philosopher i+1 (fork n-1 is
// between the last philosopher and philosopher 0). To eat, philosopher i
// needs both of its forks, i and (i+1) % n.
//
// The code below is the obvious solution: everybody picks up fork i first,
// then fork i+1. It DEADLOCKS. Run the test: it reports the deadlock and
// shows what each philosopher was doing. (make run hangs: press Ctrl+C.)
//
// PREDICT FIRST: give an order of steps in which all n philosophers are
//                stuck. Which one rule do you need to change so that the
//                circle of waiting can never close? (L08 slide 28)
//
// TODO: change dp_pick_up so it cannot deadlock. Keep one semaphore per
//       fork. Keep the call to dp_between_forks(i) between the two waits:
//       the test uses it to pause there, which makes a deadlock very likely
//       if one is possible.
//
// Test:  make test M=09 T=ex2_dining
// Run:   make run  M=09 EX=ex2_dining
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <unistd.h>

#define MAX_PHIL 16
static sem_t fork_sem[MAX_PHIL];   // fork i lies between philosopher i and philosopher i+1
static int n;

void dp_between_forks(int i);      // provided by the test (or by main below): a short pause

void dp_init(int count) {
    n = count;
    for (int f = 0; f < n; f++) sem_init(&fork_sem[f], 0, 1);
}

void dp_destroy(void) {
    for (int f = 0; f < n; f++) sem_destroy(&fork_sem[f]);
}

void dp_pick_up(int i) {
    // TODO: this order can deadlock - fix it
    sem_wait(&fork_sem[i]);
    dp_between_forks(i);
    sem_wait(&fork_sem[(i + 1) % n]);
}

void dp_put_down(int i) {
    sem_post(&fork_sem[i]);
    sem_post(&fork_sem[(i + 1) % n]);
}

#ifndef TESTING
#define PHILS 5
#define MEALS 3
void dp_between_forks(int i) {
    (void)i;
    usleep(1000);   // holding one fork for a moment makes the deadlock easy to hit
}
static void *philosopher(void *arg) {
    int i = (int)(long)arg;
    for (int m = 1; m <= MEALS; m++) {
        dp_pick_up(i);
        printf("philosopher %d eats (meal %d)\n", i, m);
        usleep(2000);
        dp_put_down(i);
        usleep(1000);   // think
    }
    return NULL;
}
int main(void) {
    pthread_t t[PHILS];
    dp_init(PHILS);
    for (long i = 0; i < PHILS; i++) pthread_create(&t[i], NULL, philosopher, (void *)i);
    for (int i = 0; i < PHILS; i++) pthread_join(t[i], NULL);
    dp_destroy();
    printf("everyone ate %d meals: no deadlock\n", MEALS);
    return 0;
}
#endif
