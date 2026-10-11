// ANSWER - Exercise 2: Dekker's algorithm                      [L05 slides 19-21]
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
    STORE(want[i], true);                 // c_i = 0: I want to enter
    while (LOAD(want[j])) {               // the other wants in too
        if (LOAD(turn) == j) {            // ...and it is ITS turn: I back off
            STORE(want[i], false);
            while (LOAD(turn) == j)       // wait (without blocking it) for my turn
                cpu_relax();
            STORE(want[i], true);
        }
        // If it is MY turn I keep my flag up; the other thread backs off.
    }
}

void dekker_unlock(int i) {
    int j = 1 - i;
    STORE(want[i], false);
    STORE(turn, j);                       // give priority to the other thread
}

// PREDICT FIRST answer: in Attempt 4 BOTH threads back off and retry
// symmetrically, so a lock-step schedule repeats forever (livelock). In Dekker
// `turn` breaks the symmetry: only the thread whose turn it is NOT backs off,
// and it waits until turn changes, so the other thread is guaranteed to get in.
