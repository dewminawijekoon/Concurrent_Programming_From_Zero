// Lab track 4 - readers and writers with a mutex and condition variables
//                                              [Lab 02, Ex 4; L08 (semaphores) readers-writers]
//
// PREDICT FIRST: five readers keep reading, one after another, so that there
//                is almost always at least one reader inside. When does the
//                single writer get its turn?
//
// Many threads share one integer. Readers only look at it, so any number of
// readers may be inside together. A writer changes it, so a writer must be
// completely alone: no other writer and no reader.
//
// Write the four entry/exit functions using the monitor state below:
//   start_read():  wait on can_read WHILE a writer is active; then reader_count++.
//   end_read():    reader_count--; if it reached 0, wake a waiting writer.
//   start_write(): wait on can_write WHILE there are readers OR a writer is
//                  active; then writer_active = 1.
//   end_write():   writer_active = 0; wake ALL waiting readers and a writer.
// Every change and every test of the state happens while holding `mutex`.
// The reading and writing themselves happen OUTSIDE the mutex (otherwise
// readers could not overlap).
//
// This version lets new readers in whenever no writer is ACTIVE, even if a
// writer is WAITING. That is "readers' preference". Lab track 5 changes it.
//
// Test:  make test M=08 T=lab4_rw_readers_pref
// Run:   make run  M=08 EX=lab4_rw_readers_pref ARGS="5 5"   (R readers, W writers)
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

int reader_count = 0;      // readers inside right now
int writer_active = 0;     // 1 while a writer is inside
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t can_read = PTHREAD_COND_INITIALIZER;
pthread_cond_t can_write = PTHREAD_COND_INITIALIZER;

void rw_init(void) {
    reader_count = 0;
    writer_active = 0;
}

void start_read(void) {
    // TODO: lock; wait on can_read WHILE a writer is active; reader_count++; unlock.
}

void end_read(void) {
    // TODO: lock; reader_count--; if it reached 0, signal can_write; unlock.
}

void start_write(void) {
    // TODO: lock; wait on can_write WHILE reader_count > 0 OR writer_active;
    //       writer_active = 1; unlock.
}

void end_write(void) {
    // TODO: lock; writer_active = 0; broadcast can_read and signal can_write; unlock.
}

#ifndef TESTING
#include "rw_demo.h"
int main(int argc, char **argv) { return rw_demo(argc, argv, "readers' preference"); }
#endif
