// Exercise 1 (warm-up) - Bill and Ben: the rendezvous      [L08 (semaphores) slides 14-18]
//
// ANSWER. Each thread first says "my part is ready" (up), then waits for
// the other one's "ready" (down). Because both threads signal BEFORE they
// wait, neither can block the other: no deadlock.
//
// Test:  make test M=07 T=ex1_bill_and_ben
// Run:   make run  M=07 EX=ex1_bill_and_ben ANS=1
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <unistd.h>

static sem_t soup_ready;       // Ben does up() when the soups are made (the slides call it "first")
static sem_t sandwiches_ready; // Bill does up() when the sandwiches are made ("second")

void cafe_init(void) {
    sem_init(&soup_ready, 0, 0);       // nothing is ready at the start
    sem_init(&sandwiches_ready, 0, 0);
}

void cafe_destroy(void) {
    sem_destroy(&soup_ready);
    sem_destroy(&sandwiches_ready);
}

// SYNCH 1: Bill has made his sandwiches.
void bill_synch(void) {
    sem_post(&sandwiches_ready);   // tell Ben: sandwiches are ready
    sem_wait(&soup_ready);         // wait until Ben's soup is ready
}

// SYNCH 2: Ben has made his soups.
void ben_synch(void) {
    sem_post(&soup_ready);         // tell Bill: soup is ready
    sem_wait(&sandwiches_ready);   // wait until Bill's sandwiches are ready
}

#ifndef TESTING
#define ROUNDS 3
static void *bill(void *arg) {
    (void)arg;
    for (int r = 1; r <= ROUNDS; r++) {
        printf("Bill: made 2 sandwiches (round %d)\n", r);
        bill_synch();
        printf("Bill: serves round %d\n", r);
    }
    return NULL;
}
static void *ben(void *arg) {
    (void)arg;
    for (int r = 1; r <= ROUNDS; r++) {
        usleep(100000);   // Ben is slow: Bill has to wait for him
        printf("Ben:  made 2 soups (round %d)\n", r);
        ben_synch();
        printf("Ben:  serves round %d\n", r);
    }
    return NULL;
}
int main(void) {
    pthread_t a, b;
    cafe_init();
    pthread_create(&a, NULL, bill, NULL);
    pthread_create(&b, NULL, ben, NULL);
    pthread_join(a, NULL);
    pthread_join(b, NULL);
    cafe_destroy();
    return 0;
}
#endif
