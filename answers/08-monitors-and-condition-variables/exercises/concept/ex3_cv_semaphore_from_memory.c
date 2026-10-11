// Exercise 3 (from memory) - the semaphore from exercise 2, no hints.
//
// Close exercise 2 and the notes. Write all four functions from memory. The
// struct is in ex2_cv_semaphore.h. The same tests as exercise 2 apply. In the
// exam you may be asked to "implement a semaphore using a monitor" in
// pseudocode, and to say why the wait is inside a while loop.
//
// Test:  make test M=08 T=ex3_cv_semaphore_from_memory
#include <pthread.h>

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

