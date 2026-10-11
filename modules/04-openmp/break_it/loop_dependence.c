// break_it/loop_dependence.c - "parallel for" on a loop that needs the previous result
// BUILD: -fopenmp
// (README section 8.4)
//
// Run:   make run M=04 EX=loop_dependence
//
// a[i] = a[i-1] + i needs a[i-1] before it can compute a[i]. With
// "#pragma omp parallel for", the iterations are split into blocks that run
// at the same time. The thread that starts at, say, i = 250000 reads
// a[249999] before the thread that owns it has computed it, so it starts
// from a wrong (old) value, and every later entry of its block is wrong too.
// OpenMP does NOT check this for you: the program compiles and runs.
#include <omp.h>
#include <stdio.h>
#include <stdlib.h>

#define N 1000000

int main(void) {
    long *a = malloc(sizeof(long) * N);
    for (int p = 1; p <= 8; p *= 2) {
        for (long i = 0; i < N; i++) a[i] = 0;
#pragma omp parallel for num_threads(p)
        for (long i = 1; i < N; i++) a[i] = a[i - 1] + i;    // WRONG with p > 1
        long bad = 0;
        for (long i = 0; i < N; i++) bad += a[i] != i * (i + 1) / 2;
        printf("%d thread%s: %7ld of %d entries wrong\n", p, p == 1 ? " " : "s", bad, N);
    }
    printf("\nFixes: a formula that uses only i (exercise 3), or a prefix sum (exercise 2).\n");
    free(a);
    return 0;
}
