// Exercise 1 (core) - Readers-writers where writers cannot starve   [L08 (semaphores) slides 29-31]
//
// The lecture's solution (slide 31) uses a lightswitch: the first reader in
// locks the room, the last one out unlocks it. Its weakness: while readers
// keep arriving, the room never becomes empty, so a writer can wait for
// ever (it STARVES). See the README, section 2.
//
// Your job: fix it with a TURNSTILE, a semaphore that starts at 1.
//   - every reader passes through it on the way in: down(turnstile); up(turnstile)
//   - a writer closes it (down) BEFORE waiting for the room, and opens it
//     (up) again when it is done
// So once a writer is waiting, newly arriving readers queue up at the
// turnstile instead of joining the readers inside.
//
// PREDICT FIRST: reader R1 is inside, writer W is waiting, reader R2
//                arrives. With slide 31, who goes first, W or R2? With the
//                turnstile? Why can't the turnstile version starve a writer?
//
// The lightswitch is given (you wrote it in module 07).
//
// Test:  make test M=09 T=ex1_rw_no_starve
// Run:   make run  M=09 EX=ex1_rw_no_starve
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <unistd.h>

// ---- the lightswitch from module 07 (finished) ----
typedef struct {
    int counter;
    sem_t mutex;
} lightswitch_t;

static void ls_init(lightswitch_t *ls) {
    ls->counter = 0;
    sem_init(&ls->mutex, 0, 1);
}
static void ls_lock(lightswitch_t *ls, sem_t *room) {
    sem_wait(&ls->mutex);
    if (++ls->counter == 1) sem_wait(room);   // first in locks the room
    sem_post(&ls->mutex);
}
static void ls_unlock(lightswitch_t *ls, sem_t *room) {
    sem_wait(&ls->mutex);
    if (--ls->counter == 0) sem_post(room);   // last out unlocks it
    sem_post(&ls->mutex);
}

// ---- the readers-writers lock ----
static sem_t room_empty;      // 1 = nobody in the room
// TODO: declare the turnstile semaphore
static lightswitch_t readers;

void rw_init(void) {
    sem_init(&room_empty, 0, 1);
    // TODO: initialise the turnstile (open)
    ls_init(&readers);
    (void)ls_lock;     // (these two lines only silence 'unused' warnings;
    (void)ls_unlock;   //  delete them once you call the functions)
}

void rw_destroy(void) {
    sem_destroy(&room_empty);
    sem_destroy(&readers.mutex);
    // TODO: destroy the turnstile
}

void reader_lock(void) {
    // TODO: pass the turnstile, then use the lightswitch
}

void reader_unlock(void) {
    // TODO
}

void writer_lock(void) {
    // TODO: close the turnstile, then wait for the room to be empty
}

void writer_unlock(void) {
    // TODO: open the turnstile and the room again
}

#ifndef TESTING
static void *reader(void *arg) {
    long id = (long)arg;
    for (int k = 0; k < 3; k++) {
        reader_lock();
        printf("reader %ld reading\n", id);
        usleep(30000);
        reader_unlock();
        usleep(1000);
    }
    return NULL;
}
static void *writer(void *arg) {
    (void)arg;
    usleep(10000);
    printf("WRITER arrives (readers are inside)\n");
    writer_lock();
    printf("WRITER writing - it did not starve\n");
    usleep(30000);
    writer_unlock();
    return NULL;
}
int main(void) {
    pthread_t t[5];
    rw_init();
    for (long i = 0; i < 4; i++) pthread_create(&t[i], NULL, reader, (void *)i);
    pthread_create(&t[4], NULL, writer, NULL);
    for (int i = 0; i < 5; i++) pthread_join(t[i], NULL);
    rw_destroy();
    return 0;
}
#endif
