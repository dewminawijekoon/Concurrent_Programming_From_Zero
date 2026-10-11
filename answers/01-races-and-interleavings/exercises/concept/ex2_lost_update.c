// Exercise 2 (core) - see a race happen every time, then fix it    [L03 slides 37-40; L01 slides 23-28]
//
// PREDICT FIRST: four threads each deposit 1 rupee into the same account
//                20,000 times. With the code below (before your fix), will the
//                balance end at 80,000? Lower? Higher? Could it ever be HIGHER?
//
// The account code below works perfectly with ONE thread. It has two races:
//   - deposit():  read the balance, then write balance + amount.
//                 Two threads can read the same old value: a "lost update".
//   - withdraw(): check "is there enough money?", then subtract.
//                 Two threads can both pass the check before either one
//                 subtracts, and the balance goes below zero.
//                 This is called "check-then-act".
// race_window() (see race_window.h) gives up the CPU at the worst moment, so
// you will see the bug on almost every run.
//
// Task:
//   1. Run the test first and watch it fail: make test M=01 T=ex2_lost_update
//      Also try: make tsan M=01 T=ex2_lost_update  (ThreadSanitizer names the racing lines)
//   2. Fix deposit() and withdraw() with the mutex inside struct account.
//      Lock before you read the balance, unlock after you write it.
//      Keep the race_window() calls inside the locked part.
//
// Test:  make test M=01 T=ex2_lost_update
// Run:   make run  M=01 EX=ex2_lost_update
#include <pthread.h>
#include <stdio.h>

#include "ex2_lost_update.h"
#include "race_window.h"

void account_init(struct account *a, long start) {
    a->balance = start;
    pthread_mutex_init(&a->lock, NULL);
}

void deposit(struct account *a, long amount) {
    pthread_mutex_lock(&a->lock);        // nobody else can read or write the balance now
    long old = a->balance;               // read
    race_window();                       // other threads may run, but they wait at the lock
    a->balance = old + amount;           // write
    pthread_mutex_unlock(&a->lock);
}

int withdraw(struct account *a, long amount) {
    int ok = 0;
    pthread_mutex_lock(&a->lock);        // the check AND the subtraction happen as one unit
    if (a->balance >= amount) {          // check
        race_window();
        a->balance -= amount;            // act
        ok = 1;
    }
    pthread_mutex_unlock(&a->lock);
    return ok;
}

long account_balance(struct account *a) {
    pthread_mutex_lock(&a->lock);
    long b = a->balance;
    pthread_mutex_unlock(&a->lock);
    return b;
}

#ifndef TESTING
#define THREADS 4
#define PER_THREAD 20000
static struct account the_account;

static void *depositor(void *arg) {
    (void)arg;
    for (int k = 0; k < PER_THREAD; k++) deposit(&the_account, 1);
    return NULL;
}

int main(void) {
    pthread_t t[THREADS];
    account_init(&the_account, 0);
    for (int i = 0; i < THREADS; i++) pthread_create(&t[i], NULL, depositor, NULL);
    for (int i = 0; i < THREADS; i++) pthread_join(t[i], NULL);
    long want = (long)THREADS * PER_THREAD;
    long got = account_balance(&the_account);
    printf("balance = %ld, expected %ld, lost %ld deposits\n", got, want, want - got);
    return 0;
}
#endif
