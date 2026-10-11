// Exercise 5 (core) - Token ring with real MPI processes   [EXTRA; README section 3.3; module 13]
// REQUIRES: mpi
//
// PREDICT FIRST: the token is one int that travels from rank to rank. Why
//                can two processes never be in the critical section together,
//                even though they share no memory and no lock?
//
// The same algorithm as exercise 4, but the nodes are MPI processes and
// "pass the token" is MPI_Send to rank (my_rank + 1) % p, "wait for the
// token" is MPI_Recv from rank (my_rank - 1 + p) % p. Rank 0 starts with the
// token. The token is an int: the number of entries still to do (p * K at the
// start). Each process enters K times; inside, call critical_section(my_rank)
// from cs_log.h. Finish the same way as exercise 4: the process that makes the
// count 0 does not pass it on again, so every message sent is also received.
//
// Test:  make test M=14 T=ex5_token_ring_mpi
// Run:   make run  M=14 EX=ex5_token_ring_mpi
#include <mpi.h>
#include <stdio.h>
#include "cs_log.h"

#define K 5            // entries per process

int main(int argc, char **argv) {
    int my_rank, p;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &p);
    int next = (my_rank + 1) % p, prev = (my_rank - 1 + p) % p;

    // ------------------------------------------------------------ your code
    // TODO: rank 0 starts with token = p * K (it may enter first, then sends it on).
    //       Then everybody loops: receive the token from prev; if it is 0,
    //       pass it on (unless you made it 0) and stop; otherwise enter if you
    //       still want to (critical_section(my_rank)), and send it to next.
    (void)next; (void)prev; (void)critical_section;

    MPI_Finalize();
    return 0;
}
