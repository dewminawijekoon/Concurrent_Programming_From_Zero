// Exercise 3 (from memory) - the semaphore from exercise 2, no hints.
//
// Close exercise 2 and the notes. Write all four functions from memory. The
// struct is in ex2_cv_semaphore.h. The same tests as exercise 2 apply. In the
// exam you may be asked to "implement a semaphore using a monitor" in
// pseudocode, and to say why the wait is inside a while loop.
//
// Test:  make test M=08 T=ex3_cv_semaphore_from_memory
#include "ex2_cv_semaphore.h"

void cv_sem_init(cv_sem *s, int value) {
    (void)s;
    (void)value;
}

void cv_sem_down(cv_sem *s) {
    (void)s;
}

void cv_sem_up(cv_sem *s) {
    (void)s;
}

void cv_sem_destroy(cv_sem *s) {
    (void)s;
}
