// Exercise 6 (core) - A hand-built tree sum and butterfly with Send/Recv
//                                                       [L10 slides 31-39]
// REQUIRES: mpi
//
// PREDICT FIRST: in the tree sum, which processes are busy in the LAST round?
//                In the butterfly, why does every process end up with the
//                total, and how many messages does that cost compared to the tree?
//
// This program uses YOUR rules from exercise 5 (it #includes that file), so
// finish exercise 5 first.
//   Part A: tree_sum() adds one value per rank; rank 0 gets the total.
//   Part B: butterfly_sum() gives the total to EVERY rank (p a power of 2).
// The program compares with MPI_Reduce / MPI_Allreduce and prints:
//   tree: <tree total> reduce: <MPI_Reduce total>            (rank 0 only)
//   rank <r> butterfly: <total>                              (every rank, if p is a power of 2)
//
// Test:  make test M=13 T=ex6_tree_reduce_mpi
// Run:   NP=8 make run M=13 EX=ex6_tree_reduce_mpi
#include <mpi.h>
#include <stdio.h>

#define TREE_SCHEDULE_NO_MAIN
#include "ex5_tree_schedule.c"   // tree_steps, tree_role, butterfly_partner

// Tree sum: returns the total on rank 0 (other ranks return a partial sum).
long tree_sum(long my_value, int my_rank, int p) {
    long sum = my_value;
    // TODO: for each round s = 0 .. tree_steps(p)-1, ask tree_role() what to do:
    //         TREE_SEND: send sum to the partner (type MPI_LONG; use s as the tag)
    //         TREE_RECV: receive one long from the partner and add it to sum
    //         TREE_IDLE: nothing
    (void)my_rank; (void)p;
    return sum;
}

// Butterfly: every rank returns the total. p must be a power of 2.
long butterfly_sum(long my_value, int my_rank, int p) {
    long sum = my_value;
    // TODO: for each round s while (1 << s) < p: swap partial sums with
    //       butterfly_partner(my_rank, s) and add what you received.
    //       Both sides send AND receive. Use MPI_Sendrecv (README section 8.3)
    //       so that neither side waits for the other forever.
    (void)my_rank; (void)p;
    return sum;
}

int main(int argc, char **argv) {
    int my_rank, p;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &p);

    long my_value = (long)(my_rank + 1) * (my_rank + 1);   // 1, 4, 9, 16, ...

    long t = tree_sum(my_value, my_rank, p);
    long r = 0;
    MPI_Reduce(&my_value, &r, 1, MPI_LONG, MPI_SUM, 0, MPI_COMM_WORLD);
    if (my_rank == 0) printf("tree: %ld reduce: %ld\n", t, r);
    fflush(stdout);

    if ((p & (p - 1)) == 0) {                               // power of 2?
        long b = butterfly_sum(my_value, my_rank, p);
        long all = 0;
        MPI_Allreduce(&my_value, &all, 1, MPI_LONG, MPI_SUM, MPI_COMM_WORLD);
        printf("rank %d butterfly: %ld (MPI_Allreduce: %ld)\n", my_rank, b, all);
    } else if (my_rank == 0) {
        printf("butterfly skipped: %d is not a power of 2\n", p);
    }
    MPI_Finalize();
    return 0;
}
