// Exercise 10 (challenge) [EXTRA] - Breaking a loop-carried dependence with MPI_Scan
//                                                       [README section 8.4; module 04]
// REQUIRES: mpi
//
// PREDICT FIRST: in   a[0] = 0;  for (i = 1; i < n; i++) a[i] = a[i-1] + i;
//                every step needs the step before it. How can rank 3 start
//                on its block before ranks 0-2 have finished theirs?
//
// Each rank owns a block of indices (block rule from exercise 3) and fills
// its part of a[] in three steps:
//   1. local pass: running total of its own indices only, starting from 0
//      (rank r's last local value is its block total T_r)
//   2. MPI_Scan with MPI_SUM on T_r gives T_0 + ... + T_r (an "inclusive
//      prefix sum"); subtract T_r to get what all LOWER ranks added up
//   3. add that offset to every local value
// Then each rank checks its values against the closed form a[i] = i(i+1)/2,
// the mismatch counts are added on rank 0 with MPI_Reduce, and the rank that
// owns a[n-1] sends it to rank 0. Rank 0 prints exactly:
//   a[n-1] = <value>
//   mismatches: <count>
//
// Test:  make test M=13 T=ex10_prefix_scan
// Run:   make run  M=13 EX=ex10_prefix_scan ARGS=1000003
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
    int my_rank, p;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &p);
    long n = argc > 1 ? atol(argv[1]) : 1000003;

    long q = n / p, extra = n % p;                            // block rule (exercise 3)
    long count = q + (my_rank < extra ? 1 : 0);
    long first = my_rank * q + (my_rank < extra ? my_rank : extra);
    long long *a = malloc((count > 0 ? count : 1) * sizeof *a);   // a[first .. first+count-1]

    // ------------------------------------------------------------ your code
    // 1. local running totals of my own indices (index 0 adds 0, which is fine)
    long long run = 0;
    for (long j = 0; j < count; j++) { run += first + j; a[j] = run; }
    long long my_total = run;

    // 2. inclusive prefix sum of the block totals, minus my own total
    long long upto_me = 0;
    MPI_Scan(&my_total, &upto_me, 1, MPI_LONG_LONG, MPI_SUM, MPI_COMM_WORLD);
    long long offset = upto_me - my_total;

    // 3. shift my values
    for (long j = 0; j < count; j++) a[j] += offset;

    // ------------------------------------------------------------ given: check
    long wrong = 0, all_wrong = 0;
    for (long j = 0; j < count; j++) {
        long long i = first + j;
        wrong += a[j] != i * (i + 1) / 2;
    }
    MPI_Reduce(&wrong, &all_wrong, 1, MPI_LONG, MPI_SUM, 0, MPI_COMM_WORLD);
    int owner = (int)(n - 1 < extra * (q + 1) ? (n - 1) / (q + 1) : extra + (n - 1 - extra * (q + 1)) / q);
    long long last = 0;
    if (my_rank == owner) {
        last = a[count - 1];
        if (owner != 0) MPI_Send(&last, 1, MPI_LONG_LONG, 0, 0, MPI_COMM_WORLD);
    }
    if (my_rank == 0) {
        if (owner != 0) MPI_Recv(&last, 1, MPI_LONG_LONG, owner, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        printf("a[n-1] = %lld\n", last);
        printf("mismatches: %ld\n", all_wrong);
    }
    free(a);
    MPI_Finalize();
    return 0;
}
