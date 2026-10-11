// Lab track 5 - writer-priority readers and writers         [Lab 02, Ex 5]
//
// PREDICT FIRST: in this version, can a READER now wait forever? Describe the
//                stream of arrivals that would cause it.
//
// Start from your lab track 4. In track 4 a waiting writer can starve: new
// readers keep walking in past it. Fix that with one extra counter:
//
//   waiting_writers = how many writers are waiting to get in.
//
//   start_read():  wait WHILE a writer is active OR waiting_writers > 0.
//   start_write(): waiting_writers++ BEFORE waiting; wait WHILE readers are
//                  inside or a writer is active; then waiting_writers-- and
//                  writer_active = 1.
//   end_read():    as before (the last reader out wakes a writer).
//   end_write():   writer_active = 0. If writers are waiting, wake one writer.
//                  Otherwise wake ALL waiting readers.
//
// Rule this gives: once a writer is waiting, no NEW reader starts before it.
// rw_waiting_writers() is given: the test uses it to see when a writer has
// started waiting.
//
// Test:  make test M=08 T=lab5_rw_writer_priority
// Run:   make run  M=08 EX=lab5_rw_writer_priority ARGS="5 5"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

int reader_count = 0;      // readers inside right now
int writer_active = 0;     // 1 while a writer is inside
int waiting_writers = 0;   // writers that have asked to get in and are waiting
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t can_read = PTHREAD_COND_INITIALIZER;
pthread_cond_t can_write = PTHREAD_COND_INITIALIZER;

void rw_init(void) {
    reader_count = 0;
    writer_active = 0;
    waiting_writers = 0;
}

// Given: how many writers are waiting right now (read under the mutex).
int rw_waiting_writers(void) {
    pthread_mutex_lock(&mutex);
    int n = waiting_writers;
    pthread_mutex_unlock(&mutex);
    return n;
}

void start_read(void) {
    pthread_mutex_lock(&mutex);
    while (writer_active || waiting_writers > 0)   // the key change from track 4
        pthread_cond_wait(&can_read, &mutex);
    reader_count++;
    pthread_mutex_unlock(&mutex);
}

void end_read(void) {
    pthread_mutex_lock(&mutex);
    reader_count--;
    if (reader_count == 0)
        pthread_cond_signal(&can_write);
    pthread_mutex_unlock(&mutex);
}

void start_write(void) {
    pthread_mutex_lock(&mutex);
    waiting_writers++;                             // from now on, new readers hold back
    while (reader_count > 0 || writer_active)
        pthread_cond_wait(&can_write, &mutex);
    waiting_writers--;
    writer_active = 1;
    pthread_mutex_unlock(&mutex);
}

void end_write(void) {
    pthread_mutex_lock(&mutex);
    writer_active = 0;
    if (waiting_writers > 0)
        pthread_cond_signal(&can_write);           // writers first
    else
        pthread_cond_broadcast(&can_read);         // no writer waiting: let all readers in
    pthread_mutex_unlock(&mutex);
}

#ifndef TESTING
#include "rw_demo.h"
int main(int argc, char **argv) { return rw_demo(argc, argv, "writer priority"); }
#endif
