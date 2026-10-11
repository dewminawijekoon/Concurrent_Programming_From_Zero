// Interface for ex2_lost_update.c (shared by the exercise and its test)
#ifndef EX2_LOST_UPDATE_H
#define EX2_LOST_UPDATE_H
#include <pthread.h>

struct account {
    long balance;           // shared data: protect every read and write
    pthread_mutex_t lock;   // use this to protect balance
};

void account_init(struct account *a, long start);
void deposit(struct account *a, long amount);
int withdraw(struct account *a, long amount);   // 1 = done, 0 = not enough money
long account_balance(struct account *a);        // read the balance safely
#endif
