// Exercise 3 (core) - The lightswitch: a readers/writers room   [L08 (semaphores) slide 31]
//
// Readers may be in the room together. A writer must be alone: no other
// writer and no reader.
//
// The LIGHTSWITCH pattern: the first person into a dark room switches the
// light on, and the last person out switches it off. Here "light on" means
// "the room is locked for my group" (sem_wait on room), and "light off"
// means "unlock the room" (sem_post on room). A counter, protected by its
// own mutex semaphore, tells you whether you are the first in or last out.
//
// PREDICT FIRST: a reader is inside and a writer is waiting. A second
//                reader arrives. Does it go in before the writer? What does
//                that mean for a writer if readers keep arriving?
//
// TODO: write ls_lock / ls_unlock, then build the room out of them:
//   - readers share one lightswitch on the room semaphore
//   - writers sem_wait / sem_post the room semaphore directly
//
// Test:  make test M=07 T=ex3_lightswitch
// Run:   make run  M=07 EX=ex3_lightswitch
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <unistd.h>

typedef struct {
    int counter;   // how many threads of this kind are inside
    sem_t mutex;   // protects counter
} lightswitch_t;

void ls_init(lightswitch_t *ls) {
    ls->counter = 0;
    sem_init(&ls->mutex, 0, 1);
}

void ls_destroy(lightswitch_t *ls) { sem_destroy(&ls->mutex); }

void ls_lock(lightswitch_t *ls, sem_t *room) {
    // TODO: count yourself in; the FIRST one in locks the room
    (void)ls;
    (void)room;
}

void ls_unlock(lightswitch_t *ls, sem_t *room) {
    // TODO: count yourself out; the LAST one out unlocks the room
    (void)ls;
    (void)room;
}

// ---- the room ----
static sem_t room_empty;          // 1 = nobody inside
static lightswitch_t readers;

void room_init(void) {
    sem_init(&room_empty, 0, 1);
    ls_init(&readers);
}
void room_destroy(void) {
    sem_destroy(&room_empty);
    ls_destroy(&readers);
}

void reader_enter(void) { /* TODO */ }
void reader_exit(void) { /* TODO */ }
void writer_enter(void) { /* TODO */ }
void writer_exit(void) { /* TODO */ }

#ifndef TESTING
static void *reader(void *arg) {
    long id = (long)arg;
    reader_enter();
    printf("reader %ld reading\n", id);
    usleep(100000);
    printf("reader %ld done\n", id);
    reader_exit();
    return NULL;
}
static void *writer(void *arg) {
    long id = (long)arg;
    writer_enter();
    printf("WRITER %ld writing (alone)\n", id);
    usleep(100000);
    printf("WRITER %ld done\n", id);
    writer_exit();
    return NULL;
}
int main(void) {
    pthread_t t[6];
    room_init();
    pthread_create(&t[0], NULL, reader, (void *)0L);
    pthread_create(&t[1], NULL, reader, (void *)1L);
    pthread_create(&t[2], NULL, writer, (void *)0L);
    pthread_create(&t[3], NULL, reader, (void *)2L);
    pthread_create(&t[4], NULL, writer, (void *)1L);
    pthread_create(&t[5], NULL, reader, (void *)3L);
    for (int i = 0; i < 6; i++) pthread_join(t[i], NULL);
    room_destroy();
    return 0;
}
#endif
