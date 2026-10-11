// Interface shared by ex2_cv_semaphore.c and ex3_cv_semaphore_from_memory.c
#ifndef EX2_CV_SEMAPHORE_H
#define EX2_CV_SEMAPHORE_H
#include <pthread.h>

typedef struct {
    int value;                 // how many permits are left (never below 0)
    pthread_mutex_t m;         // protects value
    pthread_cond_t positive;   // "value may have become > 0"
} cv_sem;

void cv_sem_init(cv_sem *s, int value);
void cv_sem_down(cv_sem *s);   // wait until value > 0, then value--   (P, wait, acquire)
void cv_sem_up(cv_sem *s);     // value++ and wake one waiter           (V, signal, release)
void cv_sem_destroy(cv_sem *s);
#endif
