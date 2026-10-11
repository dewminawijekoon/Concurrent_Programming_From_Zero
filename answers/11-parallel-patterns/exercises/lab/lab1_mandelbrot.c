// BUILD: -fopenmp
// REQUIRES: openmp
//
// Lab track 1 - loop parallel: Mandelbrot with static vs dynamic scheduling
//                                   [Lab 06, Ex 1; L01 slides 18-19; module 04]
//
// PREDICT FIRST: rows in the middle of the picture cross the black "inside"
//                region, where every pixel runs all max_iter steps. Rows at
//                the top and bottom escape after a few steps. With 4 threads
//                and schedule(static), which thread gets the most work?
//
// For every pixel we repeat z = z*z + c until |z| > 2 or max_iter steps have
// been done. The number of steps is the pixel's value. Pixels are completely
// independent, so the loop over rows can run in parallel (a "loop parallel"
// pattern). But pixels cost VERY different amounts of time.
//
// Your tasks:
//   1. mandel_static():  copy the loops of mandel_serial() and put
//      `#pragma omp parallel for schedule(static) num_threads(nthreads)` on
//      the OUTER (row) loop.
//   2. mandel_dynamic(): the same with schedule(dynamic).
//   3. static_row_work(): WITHOUT running threads, compute how much work each
//      thread gets under schedule(static): work[t] = sum of img values over
//      the rows thread t owns. Assume the block rule from ex1 (the first
//      h % nthreads threads get one extra row). The real OpenMP split is up to
//      the compiler, but GCC uses this rule.
//
// Then run it, and answer Lab 06 Q1.1 with the table it prints.
//
// Test:  make test M=11 T=lab1_mandelbrot
// Run:   make run  M=11 EX=lab1_mandelbrot ARGS="1000 1000 1000"   (width height max_iter)
#include <omp.h>
#include <stdio.h>
#include <stdlib.h>

#include "bench.h"

// The part of the plane we draw (chosen so the picture is centred).
#define X_MIN (-2.2)
#define X_MAX 1.0
#define Y_MIN (-1.4)
#define Y_MAX 1.4

// Given: number of steps before the point (cx, cy) escapes, at most max_iter.
int mandel_point(double cx, double cy, int max_iter) {
    double x = 0, y = 0;
    int k = 0;
    while (k < max_iter && x * x + y * y <= 4.0) {
        double nx = x * x - y * y + cx;
        y = 2.0 * x * y + cy;
        x = nx;
        k++;
    }
    return k;
}

// Given: the serial version. img has h rows of w pixels; pixel (row, col) is img[row * w + col].
void mandel_serial(int *img, int w, int h, int max_iter) {
    for (int row = 0; row < h; row++)
        for (int col = 0; col < w; col++) {
            double cx = X_MIN + (X_MAX - X_MIN) * col / w;
            double cy = Y_MIN + (Y_MAX - Y_MIN) * row / h;
            img[row * w + col] = mandel_point(cx, cy, max_iter);
        }
}

void mandel_static(int *img, int w, int h, int max_iter, int nthreads) {
    // Rows are split into nthreads blocks BEFORE the loop starts.
    // row and col are declared inside the loops, so each thread has its own.
#pragma omp parallel for schedule(static) num_threads(nthreads)
    for (int row = 0; row < h; row++)
        for (int col = 0; col < w; col++) {
            double cx = X_MIN + (X_MAX - X_MIN) * col / w;
            double cy = Y_MIN + (Y_MAX - Y_MIN) * row / h;
            img[row * w + col] = mandel_point(cx, cy, max_iter);
        }
}

void mandel_dynamic(int *img, int w, int h, int max_iter, int nthreads) {
    // A free thread takes the next row while the loop runs (chunk size 1).
#pragma omp parallel for schedule(dynamic) num_threads(nthreads)
    for (int row = 0; row < h; row++)
        for (int col = 0; col < w; col++) {
            double cx = X_MIN + (X_MAX - X_MIN) * col / w;
            double cy = Y_MIN + (Y_MAX - Y_MIN) * row / h;
            img[row * w + col] = mandel_point(cx, cy, max_iter);
        }
}

void static_row_work(const int *img, int w, int h, int nthreads, long *work) {
    int q = h / nthreads, rem = h % nthreads;
    int row = 0;
    for (int t = 0; t < nthreads; t++) {
        int rows = q + (t < rem ? 1 : 0);   // this thread's block of rows
        work[t] = 0;
        for (int k = 0; k < rows; k++, row++)
            for (int col = 0; col < w; col++) work[t] += img[row * w + col];
    }
}

#ifndef TESTING
int main(int argc, char **argv) {
    int w = argc > 1 ? atoi(argv[1]) : 1000;
    int h = argc > 2 ? atoi(argv[2]) : 1000;
    int max_iter = argc > 3 ? atoi(argv[3]) : 1000;
    int *ref = malloc(sizeof(int) * (size_t)w * (size_t)h);
    int *img = malloc(sizeof(int) * (size_t)w * (size_t)h);
    printf("Mandelbrot %d x %d, max_iter %d\n", w, h, max_iter);

    // Time the serial version a few times and keep the median: the first run
    // is often slow because the CPU is still speeding up.
    int reps = bench_reps(3);
    double samples[16], t0;
    if (reps > 16) reps = 16;
    for (int r = 0; r < reps; r++) {
        t0 = bench_now();
        mandel_serial(ref, w, h, max_iter);
        samples[r] = bench_now() - t0;
    }
    double ts = bench_median(samples, reps);
    printf("serial: %.4f s (median of %d runs)\n", ts, reps);

    int counts[] = {1, 2, 4, 8};
    for (int s = 0; s < 2; s++) {
        bench_table_header(s == 0 ? "schedule(static)" : "schedule(dynamic)");
        for (int k = 0; k < 4; k++) {
            t0 = bench_now();
            if (s == 0) mandel_static(img, w, h, max_iter, counts[k]);
            else mandel_dynamic(img, w, h, max_iter, counts[k]);
            double secs = bench_now() - t0;
            bench_table_row(counts[k], secs, ts);
            for (long i = 0; i < (long)w * h; i++)
                if (img[i] != ref[i]) { printf("  image differs from serial at pixel %ld!\n", i); break; }
        }
    }
    long work[8];
    static_row_work(ref, w, h, 4, work);
    long sum = 0, max = 0;
    for (int t = 0; t < 4; t++) { sum += work[t]; if (work[t] > max) max = work[t]; }
    printf("\nschedule(static) with 4 threads - work (total steps) per thread:\n");
    for (int t = 0; t < 4; t++) printf("  thread %d: %ld\n", t, work[t]);
    printf("busiest thread / average = %.2f  (1.00 would be perfectly balanced)\n",
           sum ? (double)max * 4 / (double)sum : 1.0);
    free(ref);
    free(img);
    return 0;
}
#endif
