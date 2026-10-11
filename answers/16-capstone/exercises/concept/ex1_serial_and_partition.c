// Exercise 1 (warm-up) - The serial answer, the partitions, and imbalance
//                                            [L01 slides 17-22; L10 slides 42-44; module 13 ex3]
//
// PREDICT FIRST: with 4 threads and BLOCK partitioning of p = 0 .. N-2, which
//                thread has the most work? With CYCLIC partitioning (item p
//                goes to thread p % 4), threads 0 and 2 only ever get even
//                numbers. What does that do to their work?
//
// Fill in:
//   count_twins_serial   - the reference answer, using tw_item() from twins.h,
//                          and the total work
//   block_range          - first item and count for thread t (first M % T get one extra)
//   cyclic_owner         - which thread owns item p
//   imbalance            - (largest work) / (average work); 1.0 is perfect
// Then run it and look at the table it prints.
//
// Test:  make test M=16 T=ex1_serial_and_partition
// Run:   make run  M=16 EX=ex1_serial_and_partition ARGS="1000000 4"
#include <stdio.h>
#include <stdlib.h>
#include "twins.h"

// Count twin pairs with p + 2 <= N. Put the total work in *work.
long count_twins_serial(long N, long *work) {
    long count = 0;
    *work = 0;
    for (long p = 0; p < tw_items(N); p++) count += tw_item(p, work);
    return count;
}

// Block partitioning of M items over T threads.
void block_range(long M, int T, int t, long *first, long *count) {
    long q = M / T, extra = M % T;
    *count = q + (t < extra ? 1 : 0);
    *first = t * q + (t < extra ? t : extra);
}

// Cyclic partitioning: item p goes to thread p % T.
int cyclic_owner(long p, int T) {
    return (int)(p % T);
}

// Largest work divided by the average work.
double imbalance(const long *work, int T) {
    long max = 0, sum = 0;
    for (int t = 0; t < T; t++) {
        sum += work[t];
        if (work[t] > max) max = work[t];
    }
    if (sum == 0) return 1.0;
    return (double)max / ((double)sum / T);
}

#ifndef TESTING
int main(int argc, char **argv) {
    long N = argc > 1 ? atol(argv[1]) : 1000000;
    int T = argc > 2 ? atoi(argv[2]) : 4;
    long total;
    long c = count_twins_serial(N, &total);
    printf("N = %ld: %ld twin-prime pairs, %ld units of work\n\n", N, c, total);
    long wb[64] = {0}, wc[64] = {0}, w;
    for (int t = 0; t < T; t++) {
        long f, n;
        block_range(tw_items(N), T, t, &f, &n);
        for (long p = f; p < f + n; p++) { w = 0; tw_item(p, &w); wb[t] += w; }
    }
    for (long p = 0; p < tw_items(N); p++) { w = 0; tw_item(p, &w); wc[cyclic_owner(p, T)] += w; }
    printf("| thread | block work | cyclic work |\n|--:|--:|--:|\n");
    for (int t = 0; t < T; t++) printf("| %d | %ld | %ld |\n", t, wb[t], wc[t]);
    printf("| imbalance (max/avg) | %.2f | %.2f |\n", imbalance(wb, T), imbalance(wc, T));
    return 0;
}
#endif
