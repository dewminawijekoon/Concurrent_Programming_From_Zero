// Exercise 3 (core) - The lightswitch: a readers/writers room   [L08 (semaphores) slide 31]
//
// ANSWER. The first reader in "switches on the light" (locks the room for
// all readers); the last reader out switches it off (unlocks the room).
// Writers lock the room directly.
//
// Test:  make test M=07 T=ex3_lightswitch
// Run:   make run  M=07 EX=ex3_lightswitch ANS=1
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
    sem_wait(&ls->mutex);
    ls->counter++;
    if (ls->counter == 1)   // I am the first one in: lock the room for my group.
        sem_wait(room);     // (If a writer is inside, I wait here HOLDING ls->mutex,
                            //  so the other readers queue up behind me. That is fine.)
    sem_post(&ls->mutex);
}

void ls_unlock(lightswitch_t *ls, sem_t *room) {
    sem_wait(&ls->mutex);
    ls->counter--;
    if (ls->counter == 0)   // I am the last one out: unlock the room.
        sem_post(room);
    sem_post(&ls->mutex);
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

void reader_enter(void) { ls_lock(&readers, &room_empty); }
void reader_exit(void) { ls_unlock(&readers, &room_empty); }
void writer_enter(void) { sem_wait(&room_empty); }
void writer_exit(void) { sem_post(&room_empty); }

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
