// Exercise 3 (core) - the Banker's algorithm: safety check and requests
//                     [L11 slides 15-18]
//
// Avoidance: before granting a request, ask "if I grant this, can every
// process still finish, in SOME order, even if each one asks for its full
// declared maximum?" If yes, the state is SAFE and we grant. If no, the
// request waits.
//
//   Available[r]  free instances of r
//   Max[p][r]     the most p may ever hold (declared in advance)
//   Alloc[p][r]   what p holds now
//   Need[p][r]    = Max[p][r] - Alloc[p][r]   (what p may still ask for)
//
// Safety check (is_safe):
//   work = copy of Available; nobody is finished
//   repeat:
//     find an unfinished p with Need[p][r] <= work[r] for every r
//     if found: p can finish; work += Alloc[p]; mark p finished; append p to the sequence
//     if not found: stop
//   safe  <=>  every process got finished
//
// Request (request_ok): process p asks for req[]:
//   1. if req > Need[p] for some r: refuse (p lied about its maximum)
//   2. if req > Available for some r: refuse (p must wait)
//   3. pretend to grant: Available -= req, Alloc[p] += req
//   4. if the new state is safe: keep it, return true
//      otherwise: undo step 3, return false (p must wait)
//
// PREDICT FIRST: one resource type, 10 units. Holding / max:
//                A 3/9, B 2/4, C 2/7. Free = 3. Is it safe? In which order?
//                Now A asks for 1 more. Grant it?
//
// Test:  make test M=10 T=ex3_bankers
// Run:   make run  M=10 EX=ex3_bankers
#include <stdio.h>

#include "deadlock.h"

bool is_safe(int n, int m, const int available[MAXR], const int max[MAXP][MAXR],
             const int alloc[MAXP][MAXR], int sequence[MAXP]) {
    // TODO
    (void)n; (void)m; (void)available; (void)max; (void)alloc; (void)sequence;
    return false;
}

bool request_ok(int n, int m, int available[MAXR], const int max[MAXP][MAXR],
                int alloc[MAXP][MAXR], int p, const int req[MAXR]) {
    // TODO: steps 1-4 above. Use is_safe() for step 4.
    (void)n; (void)m; (void)available; (void)max; (void)alloc; (void)p; (void)req;
    return false;
}

#ifndef TESTING
int main(void) {
    // One resource type, 10 units: A 3/9, B 2/4, C 2/7 -> 3 free.
    int avail[MAXR] = {3};
    int max[MAXP][MAXR] = {{9}, {4}, {7}};
    int alloc[MAXP][MAXR] = {{3}, {2}, {2}};
    int seq[MAXP];
    if (is_safe(3, 1, avail, max, alloc, seq))
        printf("safe; one order: %c %c %c\n", 'A' + seq[0], 'A' + seq[1], 'A' + seq[2]);
    else
        printf("unsafe\n");
    int one[MAXR] = {1};
    printf("A asks for 1 more: %s\n",
           request_ok(3, 1, avail, max, alloc, 0, one) ? "granted" : "must wait");
    return 0;
}
#endif
