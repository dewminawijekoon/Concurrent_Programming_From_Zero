// Exercise 1 (warm-up) - Bill and Ben: the rendezvous      [L08 (semaphores) slides 14-18]
//
// Bill makes sandwiches while Ben makes soup. Then each of them serves a
// soup-and-sandwich meal. Rule: Bill may not serve until Ben's soup is
// ready, and Ben may not serve until Bill's sandwiches are ready.
// Each one runs this loop:
//
//     Bill: make sandwiches;  bill_synch();  serve;   (repeat)
//     Ben:  make soup;        ben_synch();   serve;   (repeat)
//
// This "each waits for the other at one point" pattern is called a
// RENDEZVOUS.
//
// PREDICT FIRST: you need two semaphores, both starting at 0. What goes
//                wrong if BOTH threads do their down() before their up()?
//                (Check your answer with exercises/models/m1_bill_and_ben.il.)
//
// TODO: declare two semaphores, set them up in cafe_init(), and write
//       bill_synch() and ben_synch(). Use sem_init / sem_wait / sem_post
//       (see the cheat-sheet in the README).
//
// Test:  make test M=07 T=ex1_bill_and_ben
// Run:   make run  M=07 EX=ex1_bill_and_ben
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <unistd.h>

// TODO: declare your semaphores here, e.g.  static sem_t soup_ready;

void cafe_init(void) {
    // TODO: give each semaphore its starting value with sem_init(&s, 0, value)
}

void cafe_destroy(void) {
    // TODO: sem_destroy each semaphore
}

// SYNCH 1: called by Bill after he has made his sandwiches.
void bill_synch(void) {
    // TODO
}

// SYNCH 2: called by Ben after he has made his soups.
void ben_synch(void) {
    // TODO
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
