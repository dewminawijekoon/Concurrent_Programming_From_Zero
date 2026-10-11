// Exercise 1 (warm-up) - a starting gate                    [L09 slides 9-11; Lab 02 Ex 1]
//
// PREDICT FIRST: thread A calls gate_open() BEFORE thread B calls gate_wait().
//                If gate_open() only did pthread_cond_signal() and nothing
//                else, what would happen to B? (This is called a lost wake-up.)
//
// A gate starts closed. Threads that call gate_wait() sleep until some
// thread calls gate_open(). After that the gate stays open for good:
// gate_wait() returns straight away. Think of runners at a starting line
// waiting for the starting gun.
//
// Use the mutex, the condition variable and the `is_open` flag below.
//   gate_wait():  lock; wait WHILE the gate is closed; unlock.
//   gate_open():  lock; open the gate; wake EVERY waiting thread; unlock.
// Three things to get right (the test checks each one):
//   1. Remember the state in `is_open`. A signal that nobody is waiting for
//      is simply lost; a flag is not.
//   2. Wake ALL waiters, not just one.
//   3. Wait in a while loop: pthread_cond_wait() may return even though
//      nobody signalled (a "spurious wake-up").
//
// Test:  make test M=08 T=ex1_gate
// Run:   make run  M=08 EX=ex1_gate
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>

bool is_open;                              // protected by `m`
pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t opened = PTHREAD_COND_INITIALIZER;

// Close the gate (used before each test; no threads are waiting then).
void gate_init(void) {
    pthread_mutex_lock(&m);
    is_open = false;
    pthread_mutex_unlock(&m);
}

void gate_wait(void) {
    pthread_mutex_lock(&m);
    while (!is_open)
        pthread_cond_wait(&opened, &m);
    pthread_mutex_unlock(&m);
}

void gate_open(void) {
    pthread_mutex_lock(&m);
    is_open = true;
    pthread_cond_broadcast(&opened);       // every waiting runner may go
    pthread_mutex_unlock(&m);
}

#ifndef TESTING
static void *runner(void *arg) {
    long id = (long)arg;
    printf("runner %ld: at the starting line\n", id);
    gate_wait();
    printf("runner %ld: running!\n", id);
    return NULL;
}

int main(void) {
    pthread_t t[4];
    gate_init();
    for (long i = 0; i < 4; i++) pthread_create(&t[i], NULL, runner, (void *)i);
    sleep(1);
    printf("main: bang! (opening the gate)\n");
    gate_open();
    for (int i = 0; i < 4; i++) pthread_join(t[i], NULL);
    printf("main: a late runner arrives...\n");
    gate_wait();                           // the gate is still open: no waiting
    printf("main: ...and goes straight through\n");
    return 0;
}
#endif
