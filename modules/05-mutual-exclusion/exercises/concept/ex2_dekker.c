// Exercise 2 (core) - Dekker's algorithm                       [L05 slides 19-21]
//
// PREDICT FIRST: Dekker and Attempt 4 both back off when both threads want
//                in. Why can Attempt 4 livelock but Dekker cannot?
//
// Translate the slide's pseudocode into C. The lecture numbers the processes
// 1 and 2 and uses c = 0 for "I want to enter"; here the threads are 0 and 1,
// so use: want[i] (true = wants to enter), and turn (0 or 1).
//
// Test:  make test M=05 T=ex2_dekker
#include "atomics.h"

static shared_bool want[2];
static shared_int turn;

void dekker_init(void) {
    STORE(want[0], false);
    STORE(want[1], false);
    STORE(turn, 0);
}

void dekker_lock(int i) {
    int j = 1 - i;
    // TODO: raise your flag. While the other thread wants in:
    //         if it is NOT your turn: lower your flag, wait for your turn,
    //         then raise your flag again.
    (void)j;
}

void dekker_unlock(int i) {
    int j = 1 - i;
    // TODO: lower your flag and hand the turn to the other thread
    //       (same order as the slide).
    (void)j;
}
