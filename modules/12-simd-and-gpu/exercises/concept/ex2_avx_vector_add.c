// Exercise 2 (core) [EXTRA] - AVX2 by hand: vector add, SAXPY, dot product
//                                                      [L07 slides 14, 52; module 12 README section 3.1]
// BUILD: -mavx2
// REQUIRES: avx2
//
// PREDICT FIRST: an AVX register holds 8 floats, so one _mm256_add_ps does 8
//                additions. Will vec_add_avx be 8 times faster than the
//                scalar loop? Think about it for (a) small arrays that fit in
//                the cache and (b) huge arrays that must come from main memory.
//
// Fill in the three *_avx functions using AVX intrinsics. Each one has:
//   1. a main loop that handles 8 elements per step (i += 8), and
//   2. a "remainder" loop that handles the last n % 8 elements one at a time.
// Use the "u" (unaligned) loads and stores, _mm256_loadu_ps / _mm256_storeu_ps,
// because the test passes pointers that are NOT on a 32-byte boundary.
//
// Intrinsics you need (all from <immintrin.h>):
//   __m256 v = _mm256_loadu_ps(p);       load p[0..7]
//   _mm256_storeu_ps(p, v);              store v into p[0..7]
//   _mm256_add_ps(a, b), _mm256_mul_ps(a, b)    8 additions / 8 multiplications
//   _mm256_set1_ps(x)                    8 copies of x
//   _mm256_setzero_ps()                  8 zeros
//
// Test:  make test M=12 T=ex2_avx_vector_add      (also prints a speed table)
// Run:   make run  M=12 EX=ex2_avx_vector_add
#include <immintrin.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include "bench.h"

// ---------------------------------------------------------------- given
// The scalar versions do one element per step. The attribute stops gcc from
// vectorising them behind our back, so the comparison is fair.
__attribute__((optimize("no-tree-vectorize")))
void vec_add_scalar(const float *a, const float *b, float *c, size_t n) {
    for (size_t i = 0; i < n; i++) c[i] = a[i] + b[i];
}

__attribute__((optimize("no-tree-vectorize")))
void saxpy_scalar(float alpha, const float *x, float *y, size_t n) {
    for (size_t i = 0; i < n; i++) y[i] = alpha * x[i] + y[i];
}

__attribute__((optimize("no-tree-vectorize")))
float dot_scalar(const float *a, const float *b, size_t n) {
    float s = 0;
    for (size_t i = 0; i < n; i++) s += a[i] * b[i];
    return s;
}

// Add up the 8 floats inside one register ("horizontal sum").
static float hsum8(__m256 v) {
    float t[8];
    _mm256_storeu_ps(t, v);
    return ((t[0] + t[1]) + (t[2] + t[3])) + ((t[4] + t[5]) + (t[6] + t[7]));
}

// ---------------------------------------------------------------- your code

// c[i] = a[i] + b[i]
void vec_add_avx(const float *a, const float *b, float *c, size_t n) {
    // TODO: main loop, 8 elements per step: load a[i..i+7] and b[i..i+7],
    //       add them with one _mm256_add_ps, store into c[i..i+7].
    // TODO: remainder loop for the last n % 8 elements.
    (void)a; (void)b; (void)c; (void)n;
}

// y[i] = alpha * x[i] + y[i]   ("SAXPY": Single-precision A times X Plus Y)
void saxpy_avx(float alpha, const float *x, float *y, size_t n) {
    // TODO: put alpha into all 8 lanes once (_mm256_set1_ps), then the same
    //       two loops as above, using _mm256_mul_ps and _mm256_add_ps.
    (void)alpha; (void)x; (void)y; (void)n;
}

// sum of a[i] * b[i]
float dot_avx(const float *a, const float *b, size_t n) {
    // TODO: keep 8 running sums in one register (start with _mm256_setzero_ps).
    //       Each step: multiply 8 pairs and add them to the running sums.
    //       After the loop: s = hsum8(running sums); then add the remainder.
    (void)a; (void)b; (void)n; (void)hsum8;
    return 0;
}

#ifndef TESTING
int main(void) {
    size_t n = 4096;   // 3 arrays x 16 KB: fits in the cache
    float *a = malloc(n * sizeof *a), *b = malloc(n * sizeof *b), *c = malloc(n * sizeof *c);
    for (size_t i = 0; i < n; i++) { a[i] = (float)(i % 13) * 0.5f; b[i] = 1.0f; }
    int reps = 20000;
    double t0 = bench_now();
    for (int r = 0; r < reps; r++) vec_add_scalar(a, b, c, n);
    double ts = bench_now() - t0;
    t0 = bench_now();
    for (int r = 0; r < reps; r++) vec_add_avx(a, b, c, n);
    double tv = bench_now() - t0;
    printf("vector add, n=%zu, %d repeats: scalar %.4f s, AVX %.4f s, speedup %.1fx\n",
           n, reps, ts, tv, ts / tv);
    printf("dot: scalar %.1f, AVX %.1f (tiny differences come from a different order of additions)\n",
           dot_scalar(a, b, n), dot_avx(a, b, n));
    free(a); free(b); free(c);
    return 0;
}
#endif
