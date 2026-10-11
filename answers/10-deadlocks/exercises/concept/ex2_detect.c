// ANSWER - Exercise 2 (core) - deadlock detection with several instances per resource
//                     [L11 slides 11-12]
//
// When there are several identical copies of a resource (3 printers, 5 tape
// drives), a cycle in the graph is no longer enough to prove a deadlock. We
// use four tables instead (m resource types, n processes):
//
//   E[r]     existing:   how many instances of resource r exist in total
//   A[r]     available:  how many are free right now  (A = E - column sums of C)
//   C[p][r]  current allocation: how many instances of r process p holds
//   R[p][r]  requests:   how many more instances of r process p is waiting for
//
// The detection algorithm is optimistic: "could every process still finish?"
//   1. Look for an unmarked process p whose whole request row fits in A
//      (R[p][r] <= A[r] for every r).
//   2. If there is one, pretend p gets what it asked for, runs to the end and
//      gives back everything it holds: add row C[p] to A. Mark p. Go to 1.
//   3. If there is none, stop. Every unmarked process is deadlocked.
//
// PREDICT FIRST: E = (4, 2), C = P1:(2,0) P2:(1,1) P3:(1,1), R = P1:(0,1)
//                P2:(1,0) P3:(0,0). Compute A first. Is anyone deadlocked?
//                (It is the first test.)
//
// Implement detect_deadlock() (see deadlock.h for the exact contract).
// Do not change the caller's arrays (they are const): copy what you modify.
//
// Test:  make test M=10 T=ex2_detect
// Run:   make run  M=10 EX=ex2_detect
#include <stdio.h>

#include "deadlock.h"

int detect_deadlock(int n, int m, const int existing[MAXR], const int alloc[MAXP][MAXR],
                    const int request[MAXP][MAXR], bool deadlocked[MAXP]) {
    int avail[MAXR];
    bool marked[MAXP] = {false};
    // A = E minus everything that is currently handed out
    for (int r = 0; r < m; r++) {
        avail[r] = existing[r];
        for (int p = 0; p < n; p++) avail[r] -= alloc[p][r];
    }
    bool progress = true;
    while (progress) {
        progress = false;
        for (int p = 0; p < n; p++) {
            if (marked[p]) continue;
            bool fits = true;                    // step 1: does p's request fit in A?
            for (int r = 0; r < m; r++)
                if (request[p][r] > avail[r]) fits = false;
            if (fits) {                          // step 2: p can finish and give back
                for (int r = 0; r < m; r++) avail[r] += alloc[p][r];
                marked[p] = true;
                progress = true;
            }
        }
    }
    int count = 0;                               // step 3: the unmarked ones are stuck
    for (int p = 0; p < MAXP; p++) {
        deadlocked[p] = p < n && !marked[p];
        if (deadlocked[p]) count++;
    }
    return count;
}

#ifndef TESTING
int main(void) {
    int E[MAXR] = {4, 2};
    int C[MAXP][MAXR] = {{2, 0}, {1, 1}, {1, 1}};
    int R[MAXP][MAXR] = {{0, 1}, {1, 0}, {0, 0}};
    bool dead[MAXP];
    int k = detect_deadlock(3, 2, E, C, R, dead);
    printf("%d deadlocked process(es):", k);
    for (int p = 0; p < 3; p++) if (dead[p]) printf(" P%d", p + 1);
    printf("\n");
    return 0;
}
#endif
