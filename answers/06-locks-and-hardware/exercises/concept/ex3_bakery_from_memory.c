// ANSWER - Exercise 3 (from memory) - the Bakery lock.
// This version writes the waiting condition exactly like the slide:
// "wait while there is some k with flag[k] and (label[k], k) before (label[i], i)".
#include "nlock.h"

static int n;
static shared_bool flag[MAX_THREADS];
static shared_long label[MAX_THREADS];

void bakery_init(int count) {
    n = count;
    for (int k = 0; k < MAX_THREADS; k++) {
        STORE(flag[k], false);
        STORE(label[k], 0);
    }
}

void bakery_doorway(int i) {
    STORE(flag[i], true);
    long biggest = 0;
    for (int k = 0; k < n; k++)
        if (LOAD(label[k]) > biggest) biggest = LOAD(label[k]);
    STORE(label[i], biggest + 1);
}

// Is there a thread that wants in and comes before me?
static bool someone_before_me(int i) {
    long mine = LOAD(label[i]);
    for (int k = 0; k < n; k++) {
        if (k == i || !LOAD(flag[k])) continue;
        long theirs = LOAD(label[k]);
        if (theirs < mine || (theirs == mine && k < i)) return true;
    }
    return false;
}

void bakery_wait(int i) {
    unsigned spins = 0;
    while (someone_before_me(i)) spin_pause(&spins);
}

void bakery_lock(int i) {
    bakery_doorway(i);
    bakery_wait(i);
}

void bakery_unlock(int i) {
    STORE(flag[i], false);
}
