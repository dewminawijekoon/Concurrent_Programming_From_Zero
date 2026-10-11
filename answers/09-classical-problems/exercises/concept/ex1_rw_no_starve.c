// Exercise 1 (core) - Readers-writers where writers cannot starve   [L08 (semaphores) slides 29-31]
//
// ANSWER. A turnstile in front of everything. A waiting writer HOLDS the
// turnstile, so readers that arrive after it queue up behind it instead of
// joining the readers inside. When the last old reader leaves, the writer
// gets the room.
//
// Test:  make test M=09 T=ex1_rw_no_starve
// Run:   make run  M=09 EX=ex1_rw_no_starve ANS=1
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
static sem_t turnstile;       // 1 = open; a waiting writer holds it at 0
static lightswitch_t readers;

void rw_init(void) {
    sem_init(&room_empty, 0, 1);
    sem_init(&turnstile, 0, 1);
    ls_init(&readers);
}

void rw_destroy(void) {
    sem_destroy(&room_empty);
    sem_destroy(&turnstile);
    sem_destroy(&readers.mutex);
}

void reader_lock(void) {
    sem_wait(&turnstile);     // pass the turnstile (blocked while a writer waits)...
    sem_post(&turnstile);     // ...and leave it open for the next one
    ls_lock(&readers, &room_empty);
}

void reader_unlock(void) { ls_unlock(&readers, &room_empty); }

void writer_lock(void) {
    sem_wait(&turnstile);     // close the turnstile: no NEW readers get past it
    sem_wait(&room_empty);    // wait for the readers already inside to leave
}

void writer_unlock(void) {
    sem_post(&turnstile);     // let the queued readers (and writers) go on
    sem_post(&room_empty);
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
