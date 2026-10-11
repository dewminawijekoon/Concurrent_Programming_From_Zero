// Exercise 9 (core) [EXTRA] - Swapping data with a partner: deadlock and three fixes
//                                                       [L10 slide 21; README section 8.3]
// REQUIRES: mpi
//
// PREDICT FIRST: in "naive" mode both partners call MPI_Send first and
//                MPI_Recv second. Try ARGS="naive 10" and ARGS="naive 1000000".
//                One finishes and one hangs forever. Which one, and why?
//                (Stop the hanging one with Ctrl-C.)
//
// Ranks are paired: 0<->1, 2<->3, ... (partner = rank ^ 1). With an odd number
// of processes the last rank has no partner and sits out. Each rank sends
// `count` doubles to its partner and receives the partner's `count` doubles.
// Rank r's data is r * 1000000 + i, so the receiver can check it.
//
// "naive" is given. Write the three safe versions:
//   ordered     - even ranks send then receive; odd ranks receive then send
//   sendrecv    - one MPI_Sendrecv call does both
//   nonblocking - MPI_Isend + MPI_Irecv, then MPI_Waitall on both requests
// Every rank with a partner prints  rank <r>: received OK  (or WRONG).
//
// Test:  make test M=13 T=ex9_exchange
// Run:   make run  M=13 EX=ex9_exchange ARGS="sendrecv 1000000"
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// given: both send first. Works only while MPI can buffer the message.
static void exchange_naive(double *out, double *in, int count, int partner) {
    MPI_Send(out, count, MPI_DOUBLE, partner, 0, MPI_COMM_WORLD);
    MPI_Recv(in, count, MPI_DOUBLE, partner, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
}

void exchange_ordered(double *out, double *in, int count, int partner, int my_rank) {
    if (my_rank % 2 == 0) {        // even: talk first
        MPI_Send(out, count, MPI_DOUBLE, partner, 0, MPI_COMM_WORLD);
        MPI_Recv(in, count, MPI_DOUBLE, partner, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    } else {                       // odd: listen first, so the even rank's Send can complete
        MPI_Recv(in, count, MPI_DOUBLE, partner, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        MPI_Send(out, count, MPI_DOUBLE, partner, 0, MPI_COMM_WORLD);
    }
}

void exchange_sendrecv(double *out, double *in, int count, int partner) {
    MPI_Sendrecv(out, count, MPI_DOUBLE, partner, 0,     // what I send, and to whom
                 in, count, MPI_DOUBLE, partner, 0,      // what I receive, and from whom
                 MPI_COMM_WORLD, MPI_STATUS_IGNORE);
}

void exchange_nonblocking(double *out, double *in, int count, int partner) {
    MPI_Request req[2];
    MPI_Irecv(in, count, MPI_DOUBLE, partner, 0, MPI_COMM_WORLD, &req[0]);   // start, don't wait
    MPI_Isend(out, count, MPI_DOUBLE, partner, 0, MPI_COMM_WORLD, &req[1]);  // start, don't wait
    MPI_Waitall(2, req, MPI_STATUSES_IGNORE);   // now wait for both; don't touch the buffers before this
}

int main(int argc, char **argv) {
    int my_rank, p;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &p);
    const char *mode = argc > 1 ? argv[1] : "sendrecv";
    int count = argc > 2 ? atoi(argv[2]) : 1000000;
    int partner = my_rank ^ 1;

    if (partner < p) {
        double *out = malloc(count * sizeof *out), *in = malloc(count * sizeof *in);
        for (int i = 0; i < count; i++) { out[i] = my_rank * 1000000.0 + i; in[i] = -1; }
        if (strcmp(mode, "naive") == 0) exchange_naive(out, in, count, partner);
        else if (strcmp(mode, "ordered") == 0) exchange_ordered(out, in, count, partner, my_rank);
        else if (strcmp(mode, "sendrecv") == 0) exchange_sendrecv(out, in, count, partner);
        else if (strcmp(mode, "nonblocking") == 0) exchange_nonblocking(out, in, count, partner);
        else if (my_rank == 0) printf("unknown mode %s\n", mode);
        int wrong = 0;
        for (int i = 0; i < count; i++) wrong += in[i] != partner * 1000000.0 + i;
        printf("rank %d: received %s\n", my_rank, wrong ? "WRONG" : "OK");
        free(out); free(in);
    }
    MPI_Finalize();
    return 0;
}
