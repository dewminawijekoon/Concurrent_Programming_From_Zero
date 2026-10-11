// Exercise 2 (warm-up) - Multiplex: at most N threads inside   [L09 slide 27; Lab 04 intro]
//
// ANSWER. A semaphore that starts at N is a pile of N tickets. Taking a
// ticket is sem_wait, giving it back is sem_post. When all N tickets are
// taken, the next thread waits.
//
// Test:  make test M=07 T=ex2_multiplex
// Run:   make run  M=07 EX=ex2_multiplex ANS=1
#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdatomic.h>
#include <stdio.h>
#include <unistd.h>

static sem_t tickets;   // number of free places inside

void mplex_init(int n) { sem_init(&tickets, 0, (unsigned)n); }
void mplex_destroy(void) { sem_destroy(&tickets); }

void mplex_enter(void) { sem_wait(&tickets); }   // take a ticket (wait if none is left)
void mplex_exit(void) { sem_post(&tickets); }    // give the ticket back

// Take a ticket only if one is free right now. Never waits.
int mplex_try_enter(void) {
    // sem_trywait returns 0 if it took a ticket; -1 with errno == EAGAIN if the
    // value was 0. (EINTR cannot happen here because trywait never sleeps.)
    return sem_trywait(&tickets) == 0;
}

#ifndef TESTING
#define N 3
#define THREADS 8
static atomic_int inside;
static void *worker(void *arg) {
    long id = (long)arg;
    mplex_enter();
    int now = atomic_fetch_add(&inside, 1) + 1;
    printf("thread %ld in  (inside now: %d)\n", id, now);
    usleep(100000);
    now = atomic_fetch_sub(&inside, 1) - 1;
    printf("thread %ld out (inside now: %d)\n", id, now);
    mplex_exit();
    return NULL;
}
int main(void) {
    pthread_t t[THREADS];
    mplex_init(N);
    for (long i = 0; i < THREADS; i++) pthread_create(&t[i], NULL, worker, (void *)i);
    for (int i = 0; i < THREADS; i++) pthread_join(t[i], NULL);
    mplex_destroy();
    printf("never more than %d inside? look at the numbers above\n", N);
    return 0;
}
#endif
