// Interface for ex6_bank_transfer.c
#ifndef EX6_BANK_TRANSFER_H
#define EX6_BANK_TRANSFER_H
#include <pthread.h>
#include <stdbool.h>

typedef struct {
    int id;                  // unique; used to decide the locking order
    long balance;            // protected by lock
    pthread_mutex_t lock;
} account;

void account_init(account *a, int id, long balance);
bool transfer(account *from, account *to, long amount);
long total_balance(account *accts, int n);   // accts[i].id must be i
#endif
