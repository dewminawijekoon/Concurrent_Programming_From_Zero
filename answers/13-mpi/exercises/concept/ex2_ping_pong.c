// Exercise 2 (warm-up) - Ping-pong between two ranks, and how long a message takes
//                                                       [L10 slides 15-21; [EXTRA] MPI_Wtime]
// REQUIRES: mpi
//
// PREDICT FIRST: a message between two processes on the same machine takes
//                about a microsecond. Sending 1 MB takes much longer. Which
//                part of the cost is fixed per message, and which part grows
//                with the size?
//
// Ranks 0 and 1 pass a "ball" (one int) back and forth ROUNDS times. Each
// time a process receives the ball it adds 1, then sends it back. Rank 0
// throws first (ball = 0 + 1 = 1). After ROUNDS round trips the ball has
// been increased 2 * ROUNDS times.
// Ranks 2, 3, ... take no part: they just start and stop MPI.
//
// Rank 0 then prints exactly:   final ball = <value>
// and a small timing table (for information only).
//
// Test:  make test M=13 T=ex2_ping_pong
// Run:   NP=2 make run M=13 EX=ex2_ping_pong
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

#define ROUNDS 1000
#define TAG_BALL 7

// One round trip of `count` chars between ranks 0 and 1 (used for the timing table).
static void bounce_bytes(char *buf, int count, int my_rank) {
    if (my_rank == 0) {
        MPI_Send(buf, count, MPI_CHAR, 1, 0, MPI_COMM_WORLD);
        MPI_Recv(buf, count, MPI_CHAR, 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    } else if (my_rank == 1) {
        MPI_Recv(buf, count, MPI_CHAR, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        MPI_Send(buf, count, MPI_CHAR, 0, 0, MPI_COMM_WORLD);
    }
}

int main(int argc, char **argv) {
    int my_rank, comm_sz;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &comm_sz);
    if (comm_sz < 2) {
        if (my_rank == 0) printf("ping-pong needs at least 2 processes (mpiexec -n 2 ...)\n");
        MPI_Finalize();
        return 1;
    }

    int ball = 0;
    double t0 = MPI_Wtime();                     // wall-clock time in seconds
    for (int r = 0; r < ROUNDS; r++) {
        if (my_rank == 0) {
            ball++;                                                      // my hit
            MPI_Send(&ball, 1, MPI_INT, 1, TAG_BALL, MPI_COMM_WORLD);
            MPI_Recv(&ball, 1, MPI_INT, 1, TAG_BALL, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        } else if (my_rank == 1) {
            MPI_Recv(&ball, 1, MPI_INT, 0, TAG_BALL, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            ball++;                                                      // my hit
            MPI_Send(&ball, 1, MPI_INT, 0, TAG_BALL, MPI_COMM_WORLD);
        }
    }
    double t1 = MPI_Wtime();
    if (my_rank == 0) {
        printf("final ball = %d\n", ball);
        printf("one round trip of 1 int: %.2f microseconds\n", (t1 - t0) / ROUNDS * 1e6);
    }

    // ------------------------------------------------------------ given: timing table
    char *buf = calloc(1 << 20, 1);
    if (my_rank == 0) printf("\n| message size (bytes) | round trip (us) | bandwidth (MB/s) |\n|--:|--:|--:|\n");
    for (int size = 1; size <= (1 << 20); size *= 16) {
        int reps = size >= (1 << 16) ? 20 : 200;
        MPI_Barrier(MPI_COMM_WORLD);
        double s = MPI_Wtime();
        for (int r = 0; r < reps; r++) bounce_bytes(buf, size, my_rank);
        double rt = (MPI_Wtime() - s) / reps;
        if (my_rank == 0) printf("| %d | %.1f | %.0f |\n", size, rt * 1e6, 2.0 * size / rt / 1e6);
    }
    free(buf);

    MPI_Finalize();
    return 0;
}
