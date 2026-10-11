// Interface for ex1_count_interleavings.c (shared by the exercise and its test)
#ifndef EX1_COUNT_INTERLEAVINGS_H
#define EX1_COUNT_INTERLEAVINGS_H

#define LIST_LINE 32            // longest interleaving string + 1
#define MAX_THREADS_COUNTED 64

// Given: "a choose b", or 0 if it does not fit in an unsigned long long.
unsigned long long binomial(int a, int b);

// Write every interleaving of A (m1 steps) and B (m2 steps) as a string of
// 'A'/'B' into out[0..]; at most max strings. Returns the full count.
int list_interleavings(int m1, int m2, char out[][LIST_LINE], int max);

// k threads, thread j has len[j] steps. 0 if the count does not fit in 64 bits.
unsigned long long count_interleavings_lengths(int k, const int len[]);

// n threads with m steps each: (n*m)! / (m!)^n. 0 if too big.
unsigned long long count_interleavings(int n, int m);
#endif
