// Lab track 3 - The barbershop: 1 barber, M waiting chairs   [Lab 04, Ex 3; L08 (semaphores) slides 32-34]
//
// ANSWER. Semaphores, as the lab lists them:
//   mutex        = 1  protects `waiting` (and `closing`)
//   customers    = 0  counts waiting customers; the barber sleeps on it
//   barber_ready = 0  the barber calls the next customer to the chair
//   seated       = 0  the customer says "I'm in the chair" (and tells the barber its id)
//   cut_done     = 0  the barber says "your haircut is finished"
//   left_chair   = 0  the customer says "I have got up and left"
// The L08 slide 33 solution has the first three. `seated` lets the barber
// know WHO he is cutting; `cut_done` makes the customer wait for the end of
// the haircut (the slide's customer leaves as soon as the barber is ready).
//
// Why `left_chair`? Without it, the barber posts cut_done and at once calls
// the next customer. If the old customer is slow to wake up, the NEW
// customer can take the old customer's cut_done token and leave before its
// own haircut has even started. (The test found exactly this bug in an
// earlier version of this answer.) A token on a shared semaphore is not
// addressed to anyone: whoever waits first takes it. So the barber waits
// until the old customer has really left before calling the next one.
//
// Test:  make test M=09 T=lab3_barbershop
// Run:   make run  M=09 EX=lab3_barbershop ANS=1 ARGS="5"
//        make run  M=09 EX=lab3_barbershop ANS=1 ARGS="table"
#include <pthread.h>
#include <semaphore.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// Provided by the test (or by main below):
void shop_event(int customer, const char *what, int value);   // "sit" (value = waiting), "balk", "done"
void shop_haircut(int customer);                              // the barber cuts this customer's hair

static sem_t mutex, customers, barber_ready, seated, cut_done, left_chair;
static int waiting, chairs, closing;
static int in_chair;   // id of the customer in the barber chair (written before up(seated))

void shop_init(int m) {
    chairs = m;
    waiting = 0;
    closing = 0;
    sem_init(&mutex, 0, 1);
    sem_init(&customers, 0, 0);
    sem_init(&barber_ready, 0, 0);
    sem_init(&seated, 0, 0);
    sem_init(&cut_done, 0, 0);
    sem_init(&left_chair, 0, 0);
}

void shop_destroy(void) {
    sem_destroy(&mutex);
    sem_destroy(&customers);
    sem_destroy(&barber_ready);
    sem_destroy(&seated);
    sem_destroy(&cut_done);
    sem_destroy(&left_chair);
}

// Returns 1 if the customer got a haircut, 0 if there was no free chair.
int customer_visit(int id) {
    sem_wait(&mutex);
    if (waiting == chairs) {             // no free chair: leave ("balk")
        shop_event(id, "balk", waiting);
        sem_post(&mutex);
        return 0;
    }
    waiting++;
    shop_event(id, "sit", waiting);
    sem_post(&customers);                // wake the barber if he sleeps
    sem_post(&mutex);

    sem_wait(&barber_ready);             // wait to be called to the chair
    in_chair = id;
    sem_post(&seated);                   // "I'm in the chair"
    sem_wait(&cut_done);                 // wait for the haircut to finish
    shop_event(id, "done", 0);
    sem_post(&left_chair);               // "I'm out of the chair"
    return 1;
}

// The barber thread runs this. It returns after shop_close().
void barber_work(void) {
    for (;;) {
        sem_wait(&customers);            // sleep until a customer (or closing time) arrives
        sem_wait(&mutex);
        if (waiting == 0 && closing) {   // only the closing "wake-up" is left
            sem_post(&mutex);
            return;
        }
        waiting--;                       // a waiting chair becomes free
        sem_post(&barber_ready);         // call the next customer
        sem_post(&mutex);
        sem_wait(&seated);               // wait until the customer sits in the chair
        shop_haircut(in_chair);
        sem_post(&cut_done);             // "you're done"
        sem_wait(&left_chair);           // wait until that customer has gone
    }
}

// Called once every customer_visit() has returned: let the barber go home.
void shop_close(void) {
    sem_wait(&mutex);
    closing = 1;
    sem_post(&mutex);
    sem_post(&customers);                // wake the barber so he sees `closing`
}

#ifndef TESTING
static int quiet;
static atomic_int served, turned_away;
static int arrival_max_us = 60000, cut_us = 20000;

void shop_event(int customer, const char *what, int value) {
    if (strcmp(what, "sit") == 0) {
        if (!quiet) printf("Customer %2d: arrived (waiting: %d/%d)\n", customer, value, chairs);
    } else if (strcmp(what, "balk") == 0) {
        atomic_fetch_add(&turned_away, 1);
        if (!quiet) printf("Customer %2d: no seats, leaving\n", customer);
    } else if (strcmp(what, "done") == 0) {
        atomic_fetch_add(&served, 1);
        if (!quiet) printf("Customer %2d: done, leaving happy\n", customer);
    }
}
void shop_haircut(int customer) {
    if (!quiet) printf("Barber: serving customer %d\n", customer);
    usleep((useconds_t)cut_us);
    if (!quiet) printf("Barber: finished customer %d\n", customer);
}

#define C 20
static void *customer_thread(void *arg) {
    int id = (int)(long)arg;
    unsigned seed = 99u + (unsigned)id;
    usleep(rand_r(&seed) % (unsigned)arrival_max_us);
    customer_visit(id);
    return NULL;
}
static void *barber_thread(void *arg) {
    (void)arg;
    barber_work();
    return NULL;
}
static void run_once(int m) {
    pthread_t b, t[C];
    atomic_store(&served, 0);
    atomic_store(&turned_away, 0);
    shop_init(m);
    pthread_create(&b, NULL, barber_thread, NULL);
    for (long i = 0; i < C; i++) pthread_create(&t[i], NULL, customer_thread, (void *)i);
    for (int i = 0; i < C; i++) pthread_join(t[i], NULL);
    shop_close();
    pthread_join(b, NULL);
    shop_destroy();
}
int main(int argc, char **argv) {
    if (argc > 1 && strcmp(argv[1], "table") == 0) {   // Lab Q3.1
        quiet = 1;
        printf("| waiting chairs M | served | turned away | served + turned away |\n");
        printf("|-----------------:|-------:|------------:|---------------------:|\n");
        int ms[] = {1, 3, 10};
        for (int k = 0; k < 3; k++) {
            run_once(ms[k]);
            printf("| %16d | %6d | %11d | %20d |\n", ms[k], atomic_load(&served),
                   atomic_load(&turned_away), atomic_load(&served) + atomic_load(&turned_away));
        }
        return 0;
    }
    int m = argc > 1 ? atoi(argv[1]) : 3;
    if (argc > 2 && strcmp(argv[2], "fast") == 0) arrival_max_us = 1000;   // Lab Q3.2
    run_once(m < 1 ? 1 : m);
    printf("Served: %d, turned away: %d, total: %d (expected %d)\n", atomic_load(&served),
           atomic_load(&turned_away), atomic_load(&served) + atomic_load(&turned_away), C);
    return 0;
}
#endif
