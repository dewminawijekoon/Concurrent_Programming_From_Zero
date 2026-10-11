// ANSWER - Exercise 6 (core) - bank transfers without deadlock
//                     [L11 slide 23 (ordered resources); L06 slide 4]
//
// Each account has its own lock, so transfers between DIFFERENT pairs of
// accounts can run at the same time. A transfer must hold BOTH accounts'
// locks, so that no money is created or lost half-way.
//
// The obvious code locks `from`, then `to`. Now two transfers run at once:
//     transfer(acct 1 -> acct 2)  locks 1, then waits for 2
//     transfer(acct 2 -> acct 1)  locks 2, then waits for 1      -> deadlock
//
// Fix it by attacking circular wait: always lock the account with the
// SMALLER id first, whichever direction the money goes.
// Also watch out for transfer(a, a): locking the same (non-recursive) mutex
// twice makes a thread wait for itself forever.
//
// PREDICT FIRST: with lock ordering, can two transfers ever wait for each
//                other? Draw the resource-allocation graph and try to make
//                a cycle.
//
// TODO: rewrite transfer(). Rules:
//   - lock both accounts, smaller id first (only one lock if from == to)
//   - if `from` has less than `amount`, move nothing and return false
//   - otherwise move the money and return true
//   - unlock both before returning
//
// Test:  make test M=10 T=ex6_bank_transfer
#include <pthread.h>
#include <stdbool.h>

#include "ex6_bank_transfer.h"

void account_init(account *a, int id, long balance) {
    a->id = id;
    a->balance = balance;
    pthread_mutex_init(&a->lock, NULL);
}

bool transfer(account *from, account *to, long amount) {
    if (from == to) {                     // one account: lock it once
        pthread_mutex_lock(&from->lock);
        bool ok = from->balance >= amount; // moving money to yourself changes nothing
        pthread_mutex_unlock(&from->lock);
        return ok;
    }
    // Lock in a global order (smaller id first), whatever the direction.
    account *first = from->id < to->id ? from : to;
    account *second = from->id < to->id ? to : from;
    pthread_mutex_lock(&first->lock);
    pthread_mutex_lock(&second->lock);
    bool ok = from->balance >= amount;     // check and move under BOTH locks
    if (ok) {
        from->balance -= amount;
        to->balance += amount;
    }
    pthread_mutex_unlock(&second->lock);
    pthread_mutex_unlock(&first->lock);
    return ok;
}

// Given: adds up every balance while holding ALL the locks, taken in id
// order (accounts[i].id == i), so it cannot deadlock with ordered transfers.
long total_balance(account *accts, int n) {
    for (int i = 0; i < n; i++) pthread_mutex_lock(&accts[i].lock);
    long sum = 0;
    for (int i = 0; i < n; i++) sum += accts[i].balance;
    for (int i = n - 1; i >= 0; i--) pthread_mutex_unlock(&accts[i].lock);
    return sum;
}
