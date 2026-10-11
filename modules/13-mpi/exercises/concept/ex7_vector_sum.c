// Exercise 7 (core) - Vector sum with Bcast, Scatter and Gather   [L10 slides 40-48]
// REQUIRES: mpi
//
// PREDICT FIRST: rank 0 holds the whole vectors. After MPI_Scatter, how many
//                elements does each rank hold? Which ones (block or cyclic)?
//
// Compute z = alpha * x + y for vectors of n = LOCAL_N * p doubles:
//   1. Only rank 0 knows alpha (it reads it from argv) -> MPI_Bcast it.
//   2. Only rank 0 builds x[i] = i and y[i] = 1000 - i -> MPI_Scatter both,
//      so each rank gets LOCAL_N neighbouring elements (block partitioning).
//   3. Each rank computes its piece of z.
//   4. MPI_Gather collects the pieces of z on rank 0, in rank order.
//   5. Rank 0 checks every element and prints exactly:
//        z[n-1] = <value with 1 decimal>
//        mismatches: <count>
//
// Test:  make test M=13 T=ex7_vector_sum
// Run:   make run  M=13 EX=ex7_vector_sum ARGS=2.5
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

#define LOCAL_N 1000

int main(int argc, char **argv) {
    int my_rank, p;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &p);
    int n = LOCAL_N * p;

    double alpha = 0.0;
    double *x = NULL, *y = NULL, *z = NULL;          // whole vectors: rank 0 only
    if (my_rank == 0) {
        alpha = argc > 1 ? atof(argv[1]) : 2.0;      // pretend only rank 0 can read input
        x = malloc(n * sizeof *x); y = malloc(n * sizeof *y); z = malloc(n * sizeof *z);
        for (int i = 0; i < n; i++) { x[i] = i; y[i] = 1000 - i; }
    }
    double local_x[LOCAL_N], local_y[LOCAL_N], local_z[LOCAL_N];

    // ------------------------------------------------------------ your code
    // TODO 1: broadcast alpha from rank 0 to everyone (MPI_Bcast).
    // TODO 2: scatter x into local_x and y into local_y (LOCAL_N doubles each, root 0).
    // TODO 3: local_z[i] = alpha * local_x[i] + local_y[i].
    // TODO 4: gather local_z from every rank into z on rank 0.
    for (int i = 0; i < LOCAL_N; i++) local_z[i] = 0;   // remove this line
    (void)local_x; (void)local_y;
    if (my_rank == 0) for (int i = 0; i < n; i++) z[i] = local_z[i % LOCAL_N];   // remove this line

    // ------------------------------------------------------------ given
    if (my_rank == 0) {
        int wrong = 0;
        for (int i = 0; i < n; i++) wrong += z[i] != alpha * i + (1000 - i);
        printf("z[n-1] = %.1f\n", z[n - 1]);
        printf("mismatches: %d\n", wrong);
        free(x); free(y); free(z);
    }
    MPI_Finalize();
    return 0;
}
