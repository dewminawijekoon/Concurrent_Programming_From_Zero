// Exercise 1 (warm-up) - Hello from every rank, then in order    [L10 slides 4-17]
// REQUIRES: mpi
//
// PREDICT FIRST: you start this program with  mpiexec -n 4 . In Part A every
//                process prints its own line. In which order do the 4 lines
//                appear? Is it the same every time? What about Part B?
//
// Part A: every process prints   direct: hello from rank <r> of <p>
// Part B: every process except 0 SENDS a short text to rank 0. Rank 0 prints
//         its own line and then RECEIVES from rank 1, 2, ..., p-1 in that
//         order and prints what it got. Each line is exactly:
//             ordered: rank <r> of <p> (pid <process id of the SENDER>)
//         The pid proves that the text really came from the other process.
//
// Test:  make test M=13 T=ex1_hello_rank
// Run:   make run  M=13 EX=ex1_hello_rank          (NP=6 make run ... for 6 processes)
#include <mpi.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define MAX_TEXT 100

int main(int argc, char **argv) {
    int my_rank = 0, comm_sz = 0;
    char text[MAX_TEXT];

    // TODO 1: start MPI, then find comm_sz and my_rank (MPI_Init, MPI_Comm_size, MPI_Comm_rank).
    // TODO 2: Part A - print the "direct:" line.
    // TODO 3: Part B - ranks other than 0 build the "ordered:" text with snprintf
    //         and send it to rank 0 (count = strlen(text) + 1, type MPI_CHAR).
    //         Rank 0 prints its own line, then receives from q = 1, 2, ...,
    //         comm_sz-1 in that order and prints each text.
    // TODO 4: shut MPI down (MPI_Finalize).
    (void)argc; (void)argv; (void)text; (void)my_rank; (void)comm_sz;
    return 0;
}
