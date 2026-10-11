// Exercise 1 (warm-up) - block, cyclic and block-cyclic partitioning
//                                        [L10 slide 43; L01 slides 18-19]
//
// PREDICT FIRST: 10 items, 4 threads. With block partitioning, which items
//                does thread 3 get? With cyclic partitioning? Write both
//                down, then run this program and compare.
//
// "Partitioning" means deciding which thread works on which items.
// Items are numbered 0 .. n-1 and threads (ranks) 0 .. p-1.
//
//   block:        each thread gets ONE run of consecutive items.
//                 Rule used in this repo: every thread gets n / p items, and
//                 the first n % p threads get one extra item.
//                 n = 10, p = 4  ->  [0,3) [3,6) [6,8) [8,10)
//   cyclic:       items are dealt out like cards: item i goes to thread i % p.
//   block-cyclic: cut the items into blocks of b, then deal out the BLOCKS
//                 like cards: item i goes to thread (i / b) % p.
//
// Implement the five functions below. No threads are needed here: these are
// the index formulas that every SPMD program (L10 slide 14) starts with.
//
// Test:  make test M=11 T=ex1_partition
// Run:   make run  M=11 EX=ex1_partition ARGS="10 4 2"
#include <stdio.h>
#include <stdlib.h>

// Thread r gets the items lo, lo+1, ..., hi-1 (so hi - lo items).
void block_range(long n, int p, int r, long *lo, long *hi) {
    // TODO: set *lo and *hi (use n / p and n % p)
    (void)n; (void)p; (void)r;
    *lo = 0;
    *hi = 0;
}

// Which thread owns item i under block partitioning?
int block_owner(long n, int p, long i) {
    // TODO: the inverse of block_range. Hint: the first (n % p) threads hold
    //       (n / p + 1) items each; the rest hold n / p items each.
    (void)n; (void)p; (void)i;
    return -1;
}

// Which thread owns item i under cyclic partitioning?
int cyclic_owner(int p, long i) {
    // TODO
    (void)p; (void)i;
    return -1;
}

// How many items does thread r get under cyclic partitioning?
long cyclic_count(long n, int p, int r) {
    // TODO: count without a loop
    (void)n; (void)p; (void)r;
    return -1;
}

// Which thread owns item i under block-cyclic partitioning with block size b?
int block_cyclic_owner(int p, long b, long i) {
    // TODO
    (void)p; (void)b; (void)i;
    return -1;
}

#ifndef TESTING
int main(int argc, char **argv) {
    long n = argc > 1 ? atol(argv[1]) : 10;
    int p = argc > 2 ? atoi(argv[2]) : 4;
    long b = argc > 3 ? atol(argv[3]) : 2;
    if (n < 0 || p < 1 || b < 1) { printf("usage: ex1_partition N P B\n"); return 2; }
    printf("n = %ld items, p = %d threads, block size b = %ld\n\n", n, p, b);
    for (int r = 0; r < p; r++) {
        long lo, hi;
        block_range(n, p, r, &lo, &hi);
        printf("thread %d  block: [%ld, %ld)  (%ld items)   cyclic: %ld items\n", r, lo, hi,
               hi - lo, cyclic_count(n, p, r));
    }
    printf("\nitem          :");
    for (long i = 0; i < n && i < 40; i++) printf("%3ld", i);
    printf("\nblock owner   :");
    for (long i = 0; i < n && i < 40; i++) printf("%3d", block_owner(n, p, i));
    printf("\ncyclic owner  :");
    for (long i = 0; i < n && i < 40; i++) printf("%3d", cyclic_owner(p, i));
    printf("\nblock-cyclic  :");
    for (long i = 0; i < n && i < 40; i++) printf("%3d", block_cyclic_owner(p, b, i));
    printf("\n");
    return 0;
}
#endif
