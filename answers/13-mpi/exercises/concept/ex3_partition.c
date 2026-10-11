// Exercise 3 (core) - Who owns which element? Block, cyclic, block-cyclic
//                                                       [L10 slides 42-44]
//
// PREDICT FIRST: 10 elements, 4 processes, block partitioning. Two ranks get
//                3 elements and two get 2. Which ranks get 3? Which elements
//                does rank 3 own? And with cyclic partitioning?
//
// No MPI needed: these are the index formulas every MPI program uses to
// split work. Exercises 4, 7 and 10 (and the capstone) use the same rules.
//
// Block partitioning: each rank gets ONE run of neighbouring elements. When
//   n is not a multiple of p, the first n % p ranks get one extra element.
//   n = 10, p = 4:   rank 0: 0 1 2 | rank 1: 3 4 5 | rank 2: 6 7 | rank 3: 8 9
// Cyclic partitioning: deal the elements like cards: element i goes to rank i % p.
//   n = 10, p = 4:   rank 0: 0 4 8 | rank 1: 1 5 9 | rank 2: 2 6 | rank 3: 3 7
// Block-cyclic with block size b: deal BLOCKS of b elements like cards.
//   n = 12, p = 3, b = 2:   owners 0 0 1 1 2 2 0 0 1 1 2 2
//
// Test:  make test M=13 T=ex3_partition
// Run:   make run  M=13 EX=ex3_partition ARGS="10 4"
#include <stdio.h>
#include <stdlib.h>

// Block: the first element of rank r and how many elements it owns.
void block_range(long n, int p, int r, long *first, long *count) {
    long q = n / p, extra = n % p;            // everyone gets q; ranks 0..extra-1 get one more
    *count = q + (r < extra ? 1 : 0);
    *first = r * q + (r < extra ? r : extra); // q per earlier rank, plus the extras handed out before me
}

// Block: which rank owns element i?
int block_owner(long i, long n, int p) {
    long q = n / p, extra = n % p;
    long big = extra * (q + 1);               // elements held by the ranks with an extra one
    if (i < big) return (int)(i / (q + 1));
    return (int)(extra + (i - big) / q);
}

// Cyclic: which rank owns element i?
int cyclic_owner(long i, int p) {
    return (int)(i % p);
}

// Cyclic: how many elements does rank r own?
long cyclic_count(long n, int p, int r) {
    return n / p + (r < n % p ? 1 : 0);       // same counts as block, different elements
}

// Block-cyclic: which rank owns element i, with blocks of b elements?
int block_cyclic_owner(long i, int p, int b) {
    return (int)((i / b) % p);                // which block, then deal blocks cyclically
}

#ifndef TESTING
int main(int argc, char **argv) {
    long n = argc > 1 ? atol(argv[1]) : 10;
    int p = argc > 2 ? atoi(argv[2]) : 4;
    printf("n = %ld elements, p = %d ranks\n\nblock:\n", n, p);
    for (int r = 0; r < p; r++) {
        long f, c;
        block_range(n, p, r, &f, &c);
        printf("  rank %d: elements %ld..%ld (%ld of them)\n", r, f, f + c - 1, c);
    }
    printf("\nelement:      ");
    for (long i = 0; i < n; i++) printf("%3ld", i);
    printf("\nblock owner:  ");
    for (long i = 0; i < n; i++) printf("%3d", block_owner(i, n, p));
    printf("\ncyclic owner: ");
    for (long i = 0; i < n; i++) printf("%3d", cyclic_owner(i, p));
    printf("\nblock-cyclic (b=2):");
    for (long i = 0; i < n; i++) printf("%3d", block_cyclic_owner(i, p, 2));
    printf("\n");
    return 0;
}
#endif
