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
    for (int s = 0; s < tree_steps(p); s++) {
        int partner;
        int role = tree_role(my_rank, s, p, &partner);
        if (role == TREE_SEND) {
            MPI_Send(&sum, 1, MPI_LONG, partner, s, MPI_COMM_WORLD);   // tag = round number
        } else if (role == TREE_RECV) {
            long other;
            MPI_Recv(&other, 1, MPI_LONG, partner, s, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            sum += other;
        }
    }
    return sum;
}

// Butterfly: every rank returns the total. p must be a power of 2.
long butterfly_sum(long my_value, int my_rank, int p) {
    long sum = my_value;
    for (int s = 0; (1 << s) < p; s++) {
        int partner = butterfly_partner(my_rank, s);
        long other;
        // Both partners send AND receive in the same round. MPI_Sendrecv does
        // both at once, so neither side can block the other (no deadlock).
        MPI_Sendrecv(&sum, 1, MPI_LONG, partner, s,
                     &other, 1, MPI_LONG, partner, s, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        sum += other;
    }
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
