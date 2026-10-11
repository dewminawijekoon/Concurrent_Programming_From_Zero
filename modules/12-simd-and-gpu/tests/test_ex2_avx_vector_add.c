// BUILD: -mavx2
// REQUIRES: avx2
// TIMEOUT: 60
#include <math.h>
#include "harness.h"
#include "bench.h"

void vec_add_scalar(const float *a, const float *b, float *c, size_t n);
void saxpy_scalar(float alpha, const float *x, float *y, size_t n);
float dot_scalar(const float *a, const float *b, size_t n);
void vec_add_avx(const float *a, const float *b, float *c, size_t n);
void saxpy_avx(float alpha, const float *x, float *y, size_t n);
float dot_avx(const float *a, const float *b, size_t n);

#define GUARD 16
#define SENTINEL -12345.0f
static const size_t sizes[] = {0, 1, 3, 7, 8, 9, 15, 16, 17, 31, 64, 100, 1003};
#define NSIZES (sizeof sizes / sizeof sizes[0])

// Arrays start 1 float after a malloc'd block, so they are NOT 32-byte aligned.
static float *make(size_t n, float *base_out[]) {
    float *base = malloc((n + GUARD + 1) * sizeof *base);
    base_out[0] = base;
    return base + 1;
}

static void fill(float *p, size_t n, int seed) {
    for (size_t i = 0; i < n; i++) p[i] = (float)((i * 7 + (size_t)seed * 3) % 23) * 0.25f - 2.0f;
}

static void test_vec_add(void) {
    for (size_t k = 0; k < NSIZES; k++) {
        size_t n = sizes[k];
        float *ba, *bb, *bc;
        float *a = make(n, &ba), *b = make(n, &bb), *c = make(n, &bc);
        fill(a, n, 1); fill(b, n, 2);
        for (size_t i = 0; i < n + GUARD; i++) c[i] = SENTINEL;
        vec_add_avx(a, b, c, n);
        size_t wrong = 0, first = 0;
        for (size_t i = 0; i < n; i++)
            if (c[i] != a[i] + b[i]) { if (!wrong) first = i; wrong++; }
        CHECK(wrong == 0, "n=%zu: %zu wrong results, first at c[%zu] (did you do the remainder loop?)", n, wrong, first);
        int overrun = 0;
        for (size_t i = n; i < n + GUARD; i++) overrun |= c[i] != SENTINEL;
        CHECK(!overrun, "n=%zu: wrote past the end of c (a full 8-wide store went beyond n)", n);
        free(ba); free(bb); free(bc);
    }
}

static void test_saxpy(void) {
    for (size_t k = 0; k < NSIZES; k++) {
        size_t n = sizes[k];
        float *bx, *by, *br;
        float *x = make(n, &bx), *y = make(n, &by), *r = make(n, &br);
        fill(x, n, 3); fill(y, n, 4);
        for (size_t i = n; i < n + GUARD; i++) y[i] = SENTINEL;
        for (size_t i = 0; i < n; i++) r[i] = y[i];
        saxpy_scalar(1.5f, x, r, n);
        saxpy_avx(1.5f, x, y, n);
        size_t wrong = 0;
        for (size_t i = 0; i < n; i++) if (fabsf(y[i] - r[i]) > 1e-5f * (1 + fabsf(r[i]))) wrong++;
        CHECK(wrong == 0, "n=%zu: %zu wrong results (y must become alpha*x + y)", n, wrong);
        int overrun = 0;
        for (size_t i = n; i < n + GUARD; i++) overrun |= y[i] != SENTINEL;
        CHECK(!overrun, "n=%zu: wrote past the end of y", n);
        free(bx); free(by); free(br);
    }
}

static void test_dot(void) {
    for (size_t k = 0; k < NSIZES; k++) {
        size_t n = sizes[k];
        float *ba, *bb;
        float *a = make(n, &ba), *b = make(n, &bb);
        fill(a, n, 5); fill(b, n, 6);
        double exact = 0;
        for (size_t i = 0; i < n; i++) exact += (double)a[i] * b[i];
        float got = dot_avx(a, b, n);
        CHECK(fabs(got - exact) <= 1e-4 * (1 + fabs(exact)), "n=%zu: dot_avx = %g, expected %g", n, got, exact);
        free(ba); free(bb);
    }
}

static void bench(void) {
    printf("\n  Speed (for information only; timings never decide PASS/FAIL)\n\n");
    printf("  | n | repeats | scalar add (s) | AVX add (s) | speedup | scalar dot (s) | AVX dot (s) | speedup |\n");
    printf("  |--:|--:|--:|--:|--:|--:|--:|--:|\n");
    size_t ns[] = {4096, (size_t)1 << 22};
    int reps[] = {20000, 20};
    for (int k = 0; k < 2; k++) {
        size_t n = ns[k];
        float *a = malloc(n * sizeof *a), *b = malloc(n * sizeof *b), *c = malloc(n * sizeof *c);
        fill(a, n, 1); fill(b, n, 2);
        volatile float sink = 0;
        double t0 = bench_now();
        for (int r = 0; r < reps[k]; r++) vec_add_scalar(a, b, c, n);
        double t_as = bench_now() - t0; t0 = bench_now();
        for (int r = 0; r < reps[k]; r++) vec_add_avx(a, b, c, n);
        double t_av = bench_now() - t0; t0 = bench_now();
        for (int r = 0; r < reps[k]; r++) sink += dot_scalar(a, b, n);
        double t_ds = bench_now() - t0; t0 = bench_now();
        for (int r = 0; r < reps[k]; r++) sink += dot_avx(a, b, n);
        double t_dv = bench_now() - t0;
        (void)sink;
        printf("  | %zu | %d | %.4f | %.4f | %.1fx | %.4f | %.4f | %.1fx |\n", n, reps[k],
               t_as, t_av, t_as / t_av, t_ds, t_dv, t_ds / t_dv);
        free(a); free(b); free(c);
    }
    printf("\n  Small n: data sits in the cache, so the 8-wide version can be several times faster.\n"
           "  Large n: both versions wait for main memory, so the gain shrinks (memory-bound).\n");
}

int main(void) {
    harness_watchdog(50);
    harness_hint("main loop: for (i = 0; i + 8 <= n; i += 8) {...}; then for (; i < n; i++) one element at a time");
    RUN_TEST(test_vec_add);
    RUN_TEST(test_saxpy);
    RUN_TEST(test_dot);
    if (h_tests_failed == 0) bench();
    return harness_report();
}
