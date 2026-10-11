// ANSWER - Lab track 3: 3x3 box blur with four schedules          [Lab 03, Ex 3]
// BUILD: -fopenmp
//
// Test:  make verify-answers M=04
// Run:   make run M=04 EX=lab3_blur ANS=1
#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bench.h"

enum sched { SCHED_STATIC, SCHED_DYNAMIC, SCHED_DYNAMIC64, SCHED_GUIDED };

// Given: fill an h x w image (stored row by row) with pseudo-random grey values 0..255.
void make_image(int *img, int h, int w, unsigned seed) {
    for (long k = 0; k < (long)h * w; k++) {
        seed = seed * 1103515245u + 12345u;
        img[k] = (int)((seed >> 16) % 256);
    }
}

// Given: the serial blur. Each inner pixel becomes the average of its 3x3
// neighbourhood. The border pixels (first/last row and column) are not written.
void blur_serial(const int *in, int *out, int h, int w) {
    for (int i = 1; i < h - 1; i++)
        for (int j = 1; j < w - 1; j++) {
            int sum = 0;
            for (int di = -1; di <= 1; di++)
                for (int dj = -1; dj <= 1; dj++) sum += in[(long)(i + di) * w + (j + dj)];
            out[(long)i * w + j] = sum / 9;
        }
}

void blur_parallel(const int *in, int *out, int h, int w, int nthreads, enum sched s) {
    // schedule(runtime) means "use the schedule set by omp_set_schedule".
    // That lets one loop serve all four policies. (Writing four copies of the
    // loop, each with its own schedule clause, in a switch, works just as well.)
    switch (s) {
    case SCHED_STATIC: omp_set_schedule(omp_sched_static, 0); break;      // 0 = default chunk
    case SCHED_DYNAMIC: omp_set_schedule(omp_sched_dynamic, 1); break;
    case SCHED_DYNAMIC64: omp_set_schedule(omp_sched_dynamic, 64); break;
    case SCHED_GUIDED: omp_set_schedule(omp_sched_guided, 0); break;
    }
    // Only the outer (row) loop is shared out. Every output pixel depends
    // only on the INPUT, which nobody writes, so the rows are independent:
    // no locks are needed. i is private automatically (it is the loop
    // variable); j, sum, di, dj are declared inside, so they are private too.
#pragma omp parallel for num_threads(nthreads) schedule(runtime)
    for (int i = 1; i < h - 1; i++)
        for (int j = 1; j < w - 1; j++) {
            int sum = 0;
            for (int di = -1; di <= 1; di++)
                for (int dj = -1; dj <= 1; dj++) sum += in[(long)(i + di) * w + (j + dj)];
            out[(long)i * w + j] = sum / 9;
        }
}

#ifndef TESTING
#define H 2000
#define W 2000
static double time_it(const int *in, int *out, int p, enum sched s) {
    double best = 1e9;
    for (int r = 0; r < 3; r++) {
        double t0 = bench_now();
        blur_parallel(in, out, H, W, p, s);
        double t = bench_now() - t0;
        if (t < best) best = t;
    }
    return best;
}

int main(void) {
    int *in = malloc(sizeof(int) * H * W), *ser = calloc(H * W, sizeof(int)), *par = calloc(H * W, sizeof(int));
    make_image(in, H, W, 42);
    double t0 = bench_now();
    blur_serial(in, ser, H, W);
    printf("serial: %.4f s\n", bench_now() - t0);
    bench_table_header("schedule(static), 2000 x 2000 image (speedup against 1 thread)");
    double t1 = time_it(in, par, 1, SCHED_STATIC);
    for (int p = 1; p <= 8; p *= 2) bench_table_row(p, time_it(in, par, p, SCHED_STATIC), t1);
    printf("\n| schedule (4 threads) | time (s) |\n|---|--:|\n");
    const char *names[] = {"static", "dynamic", "dynamic, 64", "guided"};
    for (int s = 0; s < 4; s++) printf("| %s | %.4f |\n", names[s], time_it(in, par, 4, (enum sched)s));
    printf("\noutput matches serial: %s\n", memcmp(ser, par, sizeof(int) * H * W) == 0 ? "YES" : "NO");
    free(in); free(ser); free(par);
    return 0;
}
#endif
