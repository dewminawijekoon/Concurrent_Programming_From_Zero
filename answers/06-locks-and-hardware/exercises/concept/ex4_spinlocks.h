// Interface for ex4_spinlocks.c
#ifndef EX4_SPINLOCKS_H
#define EX4_SPINLOCKS_H
#include "atomics.h"

typedef struct {
    shared_bool held;   // true while some thread holds the lock
} spinlock;

void spin_init(spinlock *l);
void tas_lock(spinlock *l);
void tas_unlock(spinlock *l);
void ttas_lock(spinlock *l);
void ttas_unlock(spinlock *l);
#endif
