// Exercise 8 (core) - Mean and standard deviation with MPI_Allreduce   [L10 slides 37-38]
// REQUIRES: mpi
//
// PREDICT FIRST: to compute the standard deviation, every process needs the
//                MEAN first. Why is MPI_Allreduce a better fit here than
//                MPI_Reduce followed by MPI_Bcast? (Hint: count the calls.)
//
// The data set has n = LOCAL_N * p numbers. Element g (g = 0 .. n-1) has the
// value (g * 37) % 101. Rank r owns the block g = r*LOCAL_N .. r*LOCAL_N + LOCAL_N-1
// and generates its own numbers: nothing is sent around at the start.
//   1. mean   = (sum of all values) / n                       -> needs one Allreduce
//   2. stddev = sqrt( (sum of (value - mean)^2) / n )           -> needs a second Allreduce
// EVERY rank prints exactly:
//   rank <r>: mean = <6 decimals> stddev = <6 decimals>
//
// Test:  make test M=13 T=ex8_allreduce_stats
// Run:   make run  M=13 EX=ex8_allreduce_stats
#include <mpi.h>
#include <math.h>
#include <stdio.h>

#define LOCAL_N 1000

int main(int argc, char **argv) {
    int my_rank, p;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &p);
    long n = (long)LOCAL_N * p;

    double v[LOCAL_N];
    for (int i = 0; i < LOCAL_N; i++) {
        long g = (long)my_rank * LOCAL_N + i;        // my block of global indices
        v[i] = (double)((g * 37) % 101);
    }

    // ------------------------------------------------------------ your code
    double local_sum = 0, sum = 0;
    for (int i = 0; i < LOCAL_N; i++) local_sum += v[i];
    MPI_Allreduce(&local_sum, &sum, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);   // everyone gets the sum
    double mean = sum / n;

    double local_sq = 0, sq = 0;
    for (int i = 0; i < LOCAL_N; i++) local_sq += (v[i] - mean) * (v[i] - mean);
    MPI_Allreduce(&local_sq, &sq, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
    double stddev = sqrt(sq / n);

    // ------------------------------------------------------------ given
    printf("rank %d: mean = %.6f stddev = %.6f\n", my_rank, mean, stddev);
    MPI_Finalize();
    return 0;
}
