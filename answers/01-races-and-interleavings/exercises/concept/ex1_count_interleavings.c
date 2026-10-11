// Exercise 1 (warm-up) - list and count interleavings           [L03 slides 32-34]
//
// PREDICT FIRST: two threads have 2 steps each. How many different orders
//                (interleavings) are there? And for 3 threads with 2 steps
//                each? Guess before you compute. (L03 slide 34 gives one of them.)
//
// An interleaving keeps each thread's own steps in order, but mixes the
// threads together. We write one as a string of thread letters:
// "ABAB" means: A's 1st step, B's 1st step, A's 2nd step, B's 2nd step.
//
// Part A - list_interleavings(m1, m2, out, max)
//   Thread A has m1 steps and thread B has m2 steps. Write every
//   interleaving into out[0], out[1], ... as a string of 'A's and 'B's
//   (with the '\0' at the end) and return how many there are.
//   Write at most `max` strings, but still return the full count.
//   Hint (recursion): an interleaving starts either with an A step (if A
//   has steps left) or with a B step (if B has steps left). Then the rest
//   is an interleaving of what is left.
//
// Part B - count_interleavings_lengths(k, len)
//   k threads; thread j has len[j] steps. Count the interleavings without
//   listing them. Idea: there are T = len[0] + ... + len[k-1] time slots.
//   Choose which slots thread 0 uses: binomial(T, len[0]) ways. Then choose
//   slots for thread 1 among the slots still free, and so on. Multiply.
//   Use the given binomial(). Return 0 if the answer does not fit in an
//   unsigned long long (binomial() returns 0 then; also watch the product).
//
// Part C - count_interleavings(n, m): n threads with m steps each.
//   This must equal the lecture's formula (n*m)! / (m!)^n.
//
// Test:  make test M=01 T=ex1_count_interleavings
// Run:   make run  M=01 EX=ex1_count_interleavings ARGS="2 2"
//        (the finished calculator is: make run M=01 EX=interleavings ARGS="3 2")
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ex1_count_interleavings.h"

// Given: "a choose b", or 0 if the result does not fit in 64 bits.
// It builds C(a-b+i, i) for i = 1..b; each division is exact.
unsigned long long binomial(int a, int b) {
    if (b < 0 || b > a) return 0;
    if (b > a - b) b = a - b;
    unsigned __int128 r = 1;
    for (int i = 1; i <= b; i++) {
        r = r * (unsigned)(a - b + i) / (unsigned)i;
        if (r > (unsigned __int128)~0ULL) return 0;
    }
    return (unsigned long long)r;
}

// Recursive helper: `prefix` holds `len` letters so far; a and b steps remain.
static void gen(int a, int b, char *prefix, int len, char out[][LIST_LINE], int max, int *count) {
    if (a == 0 && b == 0) {                  // nothing left: one complete interleaving
        if (*count < max) {
            memcpy(out[*count], prefix, (size_t)len);
            out[*count][len] = '\0';
        }
        (*count)++;
        return;
    }
    if (a > 0) {                             // next step is one of A's
        prefix[len] = 'A';
        gen(a - 1, b, prefix, len + 1, out, max, count);
    }
    if (b > 0) {                             // next step is one of B's
        prefix[len] = 'B';
        gen(a, b - 1, prefix, len + 1, out, max, count);
    }
}

int list_interleavings(int m1, int m2, char out[][LIST_LINE], int max) {
    if (m1 < 0 || m2 < 0 || m1 + m2 >= LIST_LINE) return -1;
    char prefix[LIST_LINE];
    int count = 0;
    gen(m1, m2, prefix, 0, out, max, &count);
    return count;
}

unsigned long long count_interleavings_lengths(int k, const int len[]) {
    int free_slots = 0;                       // T = total number of steps
    for (int j = 0; j < k; j++) free_slots += len[j];
    unsigned long long total = 1;
    for (int j = 0; j < k; j++) {
        // Thread j picks len[j] of the slots that are still free.
        unsigned long long ways = binomial(free_slots, len[j]);
        if (ways == 0) return 0;              // too big for 64 bits
        if (__builtin_mul_overflow(total, ways, &total)) return 0;
        free_slots -= len[j];                 // those slots are now taken
    }
    return total;
}

unsigned long long count_interleavings(int n, int m) {
    if (n < 0 || n > MAX_THREADS_COUNTED || m < 0) return 0;
    int len[MAX_THREADS_COUNTED];
    for (int j = 0; j < n; j++) len[j] = m;
    return count_interleavings_lengths(n, len);
}

#ifndef TESTING
int main(int argc, char **argv) {
    int m1 = argc > 1 ? atoi(argv[1]) : 2;
    int m2 = argc > 2 ? atoi(argv[2]) : 2;
    static char out[100][LIST_LINE];
    int c = list_interleavings(m1, m2, out, 100);
    printf("Thread A has %d steps, thread B has %d steps: %d interleavings\n", m1, m2, c);
    for (int i = 0; i < c && i < 100; i++) printf("  %s\n", out[i]);
    if (c > 100) printf("  ... (%d more)\n", c - 100);
    int len[2] = {m1, m2};
    printf("count_interleavings_lengths gives %llu\n", count_interleavings_lengths(2, len));
    printf("3 threads with 2 steps each: %llu (L03 slide 34 says 90)\n", count_interleavings(3, 2));
    return 0;
}
#endif
