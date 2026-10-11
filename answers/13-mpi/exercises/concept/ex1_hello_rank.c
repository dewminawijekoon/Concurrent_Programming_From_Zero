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
    int my_rank, comm_sz;
    char text[MAX_TEXT];

    MPI_Init(&argc, &argv);                       // set MPI up; must come first
    MPI_Comm_size(MPI_COMM_WORLD, &comm_sz);      // how many processes in total?
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);      // which one am I? (0 .. comm_sz-1)

    // Part A: everybody prints directly. The order is NOT fixed.
    printf("direct: hello from rank %d of %d\n", my_rank, comm_sz);
    fflush(stdout);

    // Part B: send to rank 0, which prints in rank order.
    if (my_rank != 0) {
        snprintf(text, MAX_TEXT, "ordered: rank %d of %d (pid %d)", my_rank, comm_sz, (int)getpid());
        // buffer, how many items (+1 for the '\0'), item type, to rank 0, tag 0, communicator
        MPI_Send(text, (int)strlen(text) + 1, MPI_CHAR, 0, 0, MPI_COMM_WORLD);
    } else {
        printf("ordered: rank %d of %d (pid %d)\n", my_rank, comm_sz, (int)getpid());
        for (int q = 1; q < comm_sz; q++) {
            // receive from rank q exactly, so the lines come out in rank order
            MPI_Recv(text, MAX_TEXT, MPI_CHAR, q, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            printf("%s\n", text);
        }
        fflush(stdout);
    }

    MPI_Finalize();                               // clean up; no MPI calls after this
    return 0;
}
