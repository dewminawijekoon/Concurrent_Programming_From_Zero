// Exercise 4 (challenge) - a tournament lock for 4 threads built from Peterson locks
//
// Peterson only works for 2 threads. Arrange 3 Peterson locks in a binary
// tree, like a knock-out tournament:
//
//                 node 1            <- winner of the final enters the CS
//               /        \
//          node 2        node 3
//          /    \        /    \
//        T0      T1    T2      T3
//
// To lock, a thread wins its match at the leaf node, then the match at the
// parent, up to the root, playing at each node as side 0 (left child) or
// side 1 (right child). To unlock, release the nodes in the REVERSE order
// (root first). Node n's children are 2n and 2n+1, so thread t starts at
// node (t + 4) / 2 with side t % 2, and moves up with side = n % 2, n = n / 2.
//
// PREDICT FIRST: why must unlock release the root first, not the leaf?
//
// Test:  make test M=05 T=ex4_tournament
#include "atomics.h"

#define NTHREADS 4
#define NNODES 4              // nodes 1..3 are used

typedef struct {
    shared_bool flag[2];
    shared_int victim;
} peterson_node;

static peterson_node node[NNODES];

static void node_lock(peterson_node *p, int side) {
    // TODO: Peterson's lock where `side` (0 or 1) plays the role of i
    (void)p; (void)side;
}

static void node_unlock(peterson_node *p, int side) {
    // TODO
    (void)p; (void)side;
}

void tournament_init(void) {
    for (int n = 0; n < NNODES; n++) {
        STORE(node[n].flag[0], false);
        STORE(node[n].flag[1], false);
        STORE(node[n].victim, 0);
    }
}

void tournament_lock(int t) {
    // TODO: climb from the leaf to the root, locking each node
    (void)t; (void)node_lock;
}

void tournament_unlock(int t) {
    // TODO: release from the root down to the leaf
    //       (hint: first record which (node, side) pairs you passed)
    (void)t; (void)node_unlock;
}
