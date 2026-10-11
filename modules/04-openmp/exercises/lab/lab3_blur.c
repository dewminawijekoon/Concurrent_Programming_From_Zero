// Lab track 3 - 3x3 box blur with four schedules                  [Lab 03, Ex 3]
// BUILD: -fopenmp
//
// Each inner pixel of the output becomes the average of the 3x3 block of
// input pixels around it. blur_serial() is given.
//
// PREDICT FIRST: every row of this image costs the same amount of work.
//                Which schedule do you expect to be fastest: static,
//                dynamic (chunk 1), dynamic with chunk 64, or guided? Why?
//
// TODO: blur_parallel(in, out, h, w, nthreads, s) - the same result as
//       blur_serial, with the ROW loop shared out by
//       "#pragma omp parallel for num_threads(nthreads)" and the schedule
//       chosen by s:
//         SCHED_STATIC    -> schedule(static)
//         SCHED_DYNAMIC   -> schedule(dynamic)        (chunk size 1)
//         SCHED_DYNAMIC64 -> schedule(dynamic, 64)
//         SCHED_GUIDED    -> schedule(guided)
//       Either write four copies of the loop in a switch, or use
//       schedule(runtime) and choose the policy first with
//       omp_set_schedule(omp_sched_dynamic, 64) and friends.
//       Do not write the border pixels (blur_serial does not either).
// The test compares your output with blur_serial on generated images.
//
// Test:  make test M=04 T=lab3_blur
// Run:   make run  M=04 EX=lab3_blur        (the lab's two tables for a 2000 x 2000 image)
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
    // TODO
    (void)in; (void)out; (void)h; (void)w; (void)nthreads; (void)s;
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
