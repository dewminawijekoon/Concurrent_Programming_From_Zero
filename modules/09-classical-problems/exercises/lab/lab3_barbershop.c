// Lab track 3 - The barbershop: 1 barber, M waiting chairs   [Lab 04, Ex 3; L08 (semaphores) slides 32-34]
//
// The rules (Lab 04 Ex 3):
//   - A customer arrives. If all M waiting chairs are taken, it leaves at
//     once ("balks"). Otherwise it sits down and waits.
//   - The barber sleeps while nobody is waiting. A customer who sits down
//     wakes him.
//   - The barber calls one waiting customer to the barber chair, cuts its
//     hair, and tells it when it is done. The customer leaves only then.
//
// You write four functions. Two "hooks" are given to you (the test records
// them, main() prints them):
//   shop_event(id, "sit", waiting)  when customer id sits down (waiting = the new count)
//   shop_event(id, "balk", waiting) when customer id leaves without a haircut
//   shop_event(id, "done", 0)       when customer id leaves after its haircut
//   shop_haircut(id)                called BY THE BARBER while he cuts customer id
//
// PREDICT FIRST: the lab lists four jobs for semaphores. Which semaphore
//                does each job, and what is its starting value? How does the
//                barber know the id of the customer in his chair?
//
// Tips: copy the structure of L08 slide 33 (mutex, customers, barber + the
//       `waiting` counter), then add what it is missing: the customer must
//       wait for the haircut to END, and the barber must learn the id.
//       shop_close() is called after every customer has left; it must make
//       barber_work() return.
//
// Test:  make test M=09 T=lab3_barbershop
// Run:   make run  M=09 EX=lab3_barbershop ARGS="5"        (5 waiting chairs)
//        make run  M=09 EX=lab3_barbershop ARGS="3 fast"   (customers arrive very fast, Lab Q3.2)
//        make run  M=09 EX=lab3_barbershop ARGS="table"    (M = 1, 3, 10, Lab Q3.1)
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

// TODO: declare your semaphores and shared variables
static int chairs;

void shop_init(int m) {
    chairs = m;
    // TODO
}

void shop_destroy(void) {
    // TODO
}

// Returns 1 if the customer got a haircut, 0 if there was no free chair.
int customer_visit(int id) {
    (void)id;
    return 0;   // TODO
}

// The barber thread runs this. It returns after shop_close().
void barber_work(void) {
    // TODO: loop: sleep until a customer is waiting; call one to the chair;
    //       shop_haircut(their id); tell them they are done.
    //       Return when the shop is closed and nobody is waiting.
}

// Called once every customer_visit() has returned: let the barber go home.
void shop_close(void) {
    // TODO
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
