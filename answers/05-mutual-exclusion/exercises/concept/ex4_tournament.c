// ANSWER - Exercise 4: tournament (tree) lock for 4 threads
#include "atomics.h"

#define NTHREADS 4
#define NNODES 4              // nodes 1..3 are used

typedef struct {
    shared_bool flag[2];
    shared_int victim;
} peterson_node;

static peterson_node node[NNODES];

static void node_lock(peterson_node *p, int side) {
    int other = 1 - side;
    STORE(p->flag[side], true);
    STORE(p->victim, side);
    while (LOAD(p->flag[other]) && LOAD(p->victim) == side)
        cpu_relax();
}

static void node_unlock(peterson_node *p, int side) {
    STORE(p->flag[side], false);
}

void tournament_init(void) {
    for (int n = 0; n < NNODES; n++) {
        STORE(node[n].flag[0], false);
        STORE(node[n].flag[1], false);
        STORE(node[n].victim, 0);
    }
}

void tournament_lock(int t) {
    int n = (t + NTHREADS) / 2, side = t % 2;
    while (n >= 1) {                       // leaf -> root
        node_lock(&node[n], side);
        side = n % 2;
        n /= 2;
    }
}

void tournament_unlock(int t) {
    int path_node[8], path_side[8], len = 0;
    int n = (t + NTHREADS) / 2, side = t % 2;
    while (n >= 1) {                       // replay the climb to record the path
        path_node[len] = n;
        path_side[len] = side;
        len++;
        side = n % 2;
        n /= 2;
    }
    for (int k = len - 1; k >= 0; k--)     // root -> leaf
        node_unlock(&node[path_node[k]], path_side[k]);
}

// PREDICT FIRST answer: suppose T0 (side 0 at the root) released its leaf
// first. T1 can now win the leaf and reach the root, also as side 0: it sets
// flag[0] = true (already true) and walks into the critical section. When T0
// then clears root flag[0], it wipes out T1's announcement as well, so T2 or
// T3 (side 1) sees flag[0] == false and enters while T1 is still inside.
// Releasing root-first means nobody from our half can reach the root until
// we have left it.
