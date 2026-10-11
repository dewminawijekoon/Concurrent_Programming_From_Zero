// Exercise 3 (observation) [EXTRA] - Let the compiler write the SIMD code
//                                                      [module 12 README section 3.1]
// BUILD: -O3 -march=native -fopt-info-vec-optimized
//
// PREDICT FIRST: which of the four loops below can the compiler turn into
//                8-at-a-time AVX code on its own? Write "yes" or "no" next to
//                each one BEFORE you run anything. Hint: look for a loop
//                where step i needs the result of step i-1.
//
// Part 1. Write the four loops (simple scalar C, one element per step).
//         The test only checks that the answers are right.
// Part 2. Run   make run M=12 EX=ex3_autovec
//         The BUILD line above asks gcc to print a note for every loop it
//         vectorised ("optimized: loop vectorized using 32 byte vectors").
//         Match the line numbers in the notes to your loops. Were your
//         predictions right?
// Part 3. sum_array is a trap. gcc may print "vectorized" for it, but look
//         at the machine code it made:
//           gcc -O3 -march=native -S -o - <this file> | grep -cE 'vaddps'
//           gcc -O3 -march=native -S -o - <this file> | grep -cE 'vaddss'
//         "vaddps" adds 8 floats at once ("packed"); "vaddss" adds just one
//         ("scalar"). Now repeat with -ffast-math added. What changed, and
//         why does gcc need your permission to change it? (Answer in the
//         quiz answers, question 9.)
//
// Test:  make test M=12 T=ex3_autovec
// Run:   make run  M=12 EX=ex3_autovec
#include <stdio.h>
#include <stdlib.h>

// y[i] = a * x[i] + y[i]. "restrict" promises the compiler that x and y do
// not overlap, so it does not have to worry that writing y changes x.
void scale_add(float *restrict y, const float *restrict x, float a, int n) {
    // TODO
    (void)y; (void)x; (void)a; (void)n;
}

// Replace every negative number by 0.
void clamp_negatives(float *x, int n) {
    // TODO
    (void)x; (void)n;
}

// Running total: after the call, x[i] = old x[0] + old x[1] + ... + old x[i].
void running_sum(float *x, int n) {
    // TODO
    (void)x; (void)n;
}

// Sum of all elements.
float sum_array(const float *x, int n) {
    // TODO
    (void)x; (void)n;
    return 0;
}

#ifndef TESTING
int main(void) {
    int n = 1000;
    float *x = malloc(n * sizeof *x), *y = malloc(n * sizeof *y);
    for (int i = 0; i < n; i++) { x[i] = (float)(i % 5) - 2; y[i] = 1; }
    scale_add(y, x, 2, n);
    clamp_negatives(y, n);
    float s = sum_array(y, n);
    running_sum(y, n);
    printf("sum = %.1f, last running total = %.1f (the two should be equal)\n", s, y[n - 1]);
    free(x); free(y);
    return 0;
}
#endif
