// Exercise 4 (core) - MPI: block, cyclic, and manager-worker   [L10; module 13]
// REQUIRES: mpi
//
// PREDICT FIRST: in "dynamic" mode rank 0 only hands out work and computes
//                nothing. With 4 processes, is it worth losing a quarter of
//                the computing power for better balance? What about 32?
//
// Usage: mpiexec -n <p> ex4_mpi <N> <block|cyclic|dynamic>
// Each rank counts the twin pairs among its items and adds up its work, then:
//   MPI_Reduce adds the counts on rank 0, and MPI_Gather collects every rank's
//   work on rank 0, which prints exactly:
//     count = <total>
//     work: <work of rank 0> <work of rank 1> ...
//     imbalance = <max / average, 2 decimals>
// Modes:
//   block   - rank r takes one run of neighbouring items (block rule)
//   cyclic  - rank r takes items r, r + p, r + 2p, ...
//   dynamic - manager-worker. Rank 0 is the manager: it waits for a READY
//             message from any worker (MPI_ANY_SOURCE) and answers with the
//             start of the next chunk of CHUNK items, or -1 when no items are
//             left. Workers loop: send READY, receive a start, stop at -1,
//             otherwise do items start .. start + CHUNK - 1 (not past the end).
//             With only 1 process, rank 0 simply does all the items itself.
//
// Test:  make test M=16 T=ex4_mpi
// Run:   make run  M=16 EX=ex4_mpi ARGS="2000000 dynamic"
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "twins.h"

#define CHUNK 1000
#define TAG_READY 1
#define TAG_WORK 2

int main(int argc, char **argv) {
    int my_rank, p;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &p);
    long N = argc > 1 ? atol(argv[1]) : 2000000;
    const char *mode = argc > 2 ? argv[2] : "block";
    long M = tw_items(N);
    long count = 0, work = 0;

    // ------------------------------------------------------------ your code
    // TODO 1: the three modes described at the top (count and work for MY items).
    // TODO 2: MPI_Reduce the counts into total on rank 0, and MPI_Gather
    //         every rank's work into all_work on rank 0 (all_work must have
    //         room for p longs on rank 0; it may be NULL on the others).
    long total = 0;
    long *all_work = my_rank == 0 ? calloc(p, sizeof *all_work) : NULL;
    (void)M; (void)work; (void)count; (void)mode;

    // ------------------------------------------------------------ given
    if (my_rank == 0) {
        long max = 0, sum = 0;
        printf("count = %ld\nwork:", total);
        for (int r = 0; r < p; r++) {
            printf(" %ld", all_work[r]);
            sum += all_work[r];
            if (all_work[r] > max) max = all_work[r];
        }
        printf("\nimbalance = %.2f\n", sum ? (double)max * p / sum : 1.0);
        free(all_work);
    }
    MPI_Finalize();
    return 0;
}
