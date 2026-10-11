// Exercise 2 (core) - Dining philosophers without deadlock   [L08 (semaphores) slides 26-28; L03 slide 11]
//
// ANSWER. The asymmetric solution of L08 slide 28: every philosopher picks
// up the LOWER-numbered of its two forks first. For philosophers 0..n-2
// that is the left fork i; for the last philosopher (n-1) it is fork 0,
// i.e. its RIGHT fork. One philosopher is "left-handed", so the circle of
// waiting (each holds one fork and waits for the next) can never close.
//
// Test:  make test M=09 T=ex2_dining
// Run:   make run  M=09 EX=ex2_dining ANS=1
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
    int left = i, right = (i + 1) % n;
    int first = left < right ? left : right;    // lower number first: breaks the circular wait
    int second = left < right ? right : left;
    sem_wait(&fork_sem[first]);
    dp_between_forks(i);
    sem_wait(&fork_sem[second]);
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
