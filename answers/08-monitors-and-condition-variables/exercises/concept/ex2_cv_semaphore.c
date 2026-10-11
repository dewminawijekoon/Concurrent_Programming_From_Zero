// Exercise 2 (core) - a counting semaphore built from a mutex and a condition variable
//                                              [L09 slides 4-11; L08 (semaphores)]
//
// PREDICT FIRST: a monitor and a semaphore can do the same jobs. If you can
//                build a semaphore out of a monitor (this exercise), what does
//                that tell you about which of the two is "more powerful"?
//
// A semaphore holds a number of permits (`value`):
//   down(): if value is 0, sleep until it is not; then take one (value--).
//   up():   give one back (value++) and wake one sleeping down(), if any.
// Build it as a small monitor: every access to `value` holds `s->m`, and a
// thread that must wait sleeps on `s->positive`.
//   - Why a while loop in down()? Because another thread may grab the permit
//     between the up() that woke you and the moment you get the mutex back.
//   - Is signal (not broadcast) enough in up()? Yes: one up() makes exactly
//     one permit, so it only needs to wake one thread.
//
// Test:  make test M=08 T=ex2_cv_semaphore
// Run:   make run  M=08 EX=ex2_cv_semaphore
#include <pthread.h>
#include <stdio.h>

#include "ex2_cv_semaphore.h"

void cv_sem_init(cv_sem *s, int value) {
    s->value = value;
    pthread_mutex_init(&s->m, NULL);
    pthread_cond_init(&s->positive, NULL);
}

void cv_sem_down(cv_sem *s) {
    pthread_mutex_lock(&s->m);
    while (s->value == 0)
        pthread_cond_wait(&s->positive, &s->m);
    s->value--;
    pthread_mutex_unlock(&s->m);
}

void cv_sem_up(cv_sem *s) {
    pthread_mutex_lock(&s->m);
    s->value++;
    pthread_cond_signal(&s->positive);
    pthread_mutex_unlock(&s->m);
}

void cv_sem_destroy(cv_sem *s) {
    pthread_mutex_destroy(&s->m);
    pthread_cond_destroy(&s->positive);
}

#ifndef TESTING
// Demo: a "multiplex" (L09 slide 27). 6 threads, but at most 2 inside at once.
#include <stdatomic.h>
#include <unistd.h>
static cv_sem room;
static atomic_int inside;

static void *visitor(void *arg) {
    long id = (long)arg;
    cv_sem_down(&room);
    int now = atomic_fetch_add(&inside, 1) + 1;
    printf("visitor %ld enters (%d inside)\n", id, now);
    usleep(200000);
    atomic_fetch_sub(&inside, 1);
    printf("visitor %ld leaves\n", id);
    cv_sem_up(&room);
    return NULL;
}

int main(void) {
    pthread_t t[6];
    cv_sem_init(&room, 2);
    for (long i = 0; i < 6; i++) pthread_create(&t[i], NULL, visitor, (void *)i);
    for (int i = 0; i < 6; i++) pthread_join(t[i], NULL);
    cv_sem_destroy(&room);
    return 0;
}
#endif
