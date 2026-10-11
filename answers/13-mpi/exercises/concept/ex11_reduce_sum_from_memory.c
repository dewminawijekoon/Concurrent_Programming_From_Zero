// Exercise 11 (from memory) - "Sum over the first N items, result on rank 0"
//                                                       [L10 slides 9-14, 33, 43]
// REQUIRES: mpi
//
// PREDICT FIRST: write down, in order, every MPI call this program needs.
//                Which ones are called by ALL processes?
//
// This is the exam's MPI outline question, done for real. Without looking at
// the other exercises, write a complete MPI program that computes
//     S = 1*1 + 2*2 + 3*3 + ... + N*N
// where N comes from argv[1] (default 1000000). Every process gets argv.
//   - split the numbers 1..N between the processes with BLOCK partitioning
//     (any N and any number of processes must work, also N < p),
//   - each process adds up its own part (use long long),
//   - combine the parts on rank 0 with MPI_Reduce,
//   - rank 0 prints exactly:   sum = <S>
//
// Test:  make test M=13 T=ex11_reduce_sum_from_memory
// Run:   make run  M=13 EX=ex11_reduce_sum_from_memory ARGS=1000
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
    int my_rank, p;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &p);
    long long N = argc > 1 ? atoll(argv[1]) : 1000000;

    // block partitioning of the items 1..N (item k has index k-1)
    long long q = N / p, extra = N % p;
    long long count = q + (my_rank < extra ? 1 : 0);
    long long first = my_rank * q + (my_rank < extra ? my_rank : extra) + 1;

    long long local = 0, total = 0;
    for (long long k = first; k < first + count; k++) local += k * k;

    // all processes call it; only rank 0 (the root) receives the total
    MPI_Reduce(&local, &total, 1, MPI_LONG_LONG, MPI_SUM, 0, MPI_COMM_WORLD);
    if (my_rank == 0) printf("sum = %lld\n", total);

    MPI_Finalize();
    return 0;
}
