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
    // TODO
    (void)argc; (void)argv;
    return 1;
}
