// Exercise 5 (core) - The tree-structured global sum, worked out in plain C
//                                                       [L10 slides 31-32, 38-39]
//
// PREDICT FIRST: 8 processes each hold one number. In the tree sum, how many
//                rounds does it take until rank 0 has the total? In round 0,
//                who sends to whom? How many rounds would 1000 processes need?
//
// The tree global sum (L10 slide 31) works in rounds. With 8 ranks:
//
//   round 0:  1->0   3->2   5->4   7->6     (odd ranks send to their left neighbour)
//   round 1:  2->0          6->4            (ranks 2 apart)
//   round 2:  4->0                          (ranks 4 apart)
//
// In round s, let d = 2^s (1, 2, 4, ...). Only ranks that are multiples of d
// still hold a partial sum.
//   - If rank % (2d) == 0, it RECEIVES from rank + d (if that rank exists).
//   - If rank % (2d) == d, it SENDS to rank - d, and is finished after that.
//   - Every other rank has already sent its sum away: it is IDLE.
// The number of rounds is the smallest s with 2^s >= p ("log2 p, rounded up").
//
// The butterfly (slide 39) is different: in round s EVERY rank swaps partial
// sums with rank XOR 2^s (flip bit s of the rank), and both add. After all
// rounds every rank has the total. We only use it when p is a power of 2.
//
// No MPI here: the test checks your rules by simulating the messages.
// Exercise 6 then uses exactly these functions in a real MPI program.
//
// Test:  make test M=13 T=ex5_tree_schedule
// Run:   make run  M=13 EX=ex5_tree_schedule ARGS=6
#include <stdio.h>
#include <stdlib.h>

enum { TREE_IDLE = 0, TREE_SEND = 1, TREE_RECV = 2 };

// Number of rounds the tree needs for p processes (0 when p == 1).
int tree_steps(int p) {
    // TODO
    (void)p;
    return 0;
}

// What does `rank` do in round `step`? Return TREE_SEND, TREE_RECV or TREE_IDLE,
// and put the other rank in *partner (set *partner = -1 when idle).
int tree_role(int rank, int step, int p, int *partner) {
    // TODO: d = 1 << step, then the three cases described at the top.
    (void)rank; (void)step; (void)p;
    *partner = -1;
    return TREE_IDLE;
}

// Butterfly partner of `rank` in round `step` (p must be a power of 2).
int butterfly_partner(int rank, int step) {
    // TODO (C's XOR operator is ^)
    (void)rank; (void)step;
    return -1;
}

#if !defined(TESTING) && !defined(TREE_SCHEDULE_NO_MAIN)
int main(int argc, char **argv) {
    int p = argc > 1 ? atoi(argv[1]) : 8;
    printf("tree sum with %d processes: %d rounds\n", p, tree_steps(p));
    for (int s = 0; s < tree_steps(p); s++) {
        printf("round %d: ", s);
        for (int r = 0; r < p; r++) {
            int partner;
            if (tree_role(r, s, p, &partner) == TREE_SEND) printf(" %d->%d ", r, partner);
        }
        printf("\n");
    }
    return 0;
}
#endif
