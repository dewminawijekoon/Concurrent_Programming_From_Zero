// ANSWER - Exercise 3: Peterson from memory (identical to ex1 without comments)
#include "atomics.h"
#include "ex1_peterson.h"

static shared_bool flag[2];
static shared_int victim;

void peterson_init(void) {
    STORE(flag[0], false);
    STORE(flag[1], false);
    STORE(victim, 0);
}

void peterson_lock(int i) {
    int j = 1 - i;
    STORE(flag[i], true);
    STORE(victim, i);
    while (LOAD(flag[j]) && LOAD(victim) == i)
        cpu_relax();
}

void peterson_unlock(int i) {
    STORE(flag[i], false);
}
