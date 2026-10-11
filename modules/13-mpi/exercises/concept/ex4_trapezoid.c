// Exercise 4 (core) - The trapezoidal rule: point-to-point, then MPI_Reduce
//                                                       [L10 slides 22-33]
// REQUIRES: mpi
//
// PREDICT FIRST: in Part A, rank 0 receives p-1 messages one after another.
//                With 1000 processes, how many steps does rank 0 need just
//                to add up the results? How could the others help?
//
// We estimate the area under f(x) = x*x between a = 0 and b = 3 with n
// trapezoids of width h = (b - a) / n. (The exact area is 9.)
//   area ~ h * [ f(x0)/2 + f(x1) + f(x2) + ... + f(x_{n-1}) + f(xn)/2 ],  xi = a + i*h
// Each process takes a block of neighbouring trapezoids (the block rule of
// exercise 3, so ANY number of processes works), computes the area of its
// block, and then the pieces are added up on rank 0 in two ways:
//   Part A: every rank except 0 sends its piece to rank 0 with MPI_Send;
//           rank 0 receives them with MPI_Recv and adds them up.
//   Part B: one call to MPI_Reduce with MPI_SUM does the same job.
// Rank 0 prints exactly two lines:
//   p2p: <result with 10 decimals>
//   reduce: <result with 10 decimals>
//
// Test:  make test M=13 T=ex4_trapezoid
// Run:   make run  M=13 EX=ex4_trapezoid ARGS="0 3 1024"
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

static double f(double x) { return x * x; }

// given: area of `count` trapezoids of width h starting at left_x
static double trap(double left_x, long count, double h) {
    if (count == 0) return 0.0;
    double right_x = left_x + count * h;
    double estimate = (f(left_x) + f(right_x)) / 2.0;
    for (long i = 1; i < count; i++) estimate += f(left_x + i * h);
    return estimate * h;
}

int main(int argc, char **argv) {
    int my_rank, comm_sz;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &comm_sz);

    double a = argc > 1 ? atof(argv[1]) : 0.0;     // every process gets the same argv
    double b = argc > 2 ? atof(argv[2]) : 3.0;
    long n = argc > 3 ? atol(argv[3]) : 1024;
    double h = (b - a) / n;                        // the same h on every process

    // ------------------------------------------------------------ your code
    // TODO 1: work out my_first and my_count (block rule) and my_area with trap().
    double my_area = 0.0;

    // TODO 2 (Part A): ranks != 0 send my_area to rank 0; rank 0 starts with
    //         its own my_area and adds one received piece from each other rank.
    double total_p2p = 0.0;

    // TODO 3 (Part B): the same total with ONE call to MPI_Reduce (root 0, MPI_SUM).
    double total_reduce = 0.0;

    (void)h; (void)my_area; (void)trap;

    // ------------------------------------------------------------ given
    if (my_rank == 0) {
        printf("p2p: %.10f\n", total_p2p);
        printf("reduce: %.10f\n", total_reduce);
    }
    MPI_Finalize();
    return 0;
}
