// Lab track 2 - Monte Carlo pi, four ways to fix the race              [Lab 03, Ex 2]
// BUILD: -fopenmp
//
// Throw random darts at the unit square. The fraction that lands inside the
// quarter circle (x*x + y*y <= 1) is about pi/4. pi_serial() is given.
//
// rand() is NOT safe to call from several threads (it keeps one hidden,
// shared state). dart() uses rand_r(&seed) instead: the state is the seed you
// pass in. Give every thread its OWN seed, 42 + its thread number, declared
// INSIDE the parallel region so that it is private.
//
// PREDICT FIRST: rank the four versions from fastest to slowest with 4
//                threads. Why?
//
// Each version: open "#pragma omp parallel num_threads(nthreads)", declare
// the seed inside, and share the dart loop with "#pragma omp for".
//   TODO A: pi_critical    - count++ inside "#pragma omp critical"
//   TODO B: pi_atomic      - count++ under "#pragma omp atomic"
//   TODO C: pi_reduction   - reduction(+:count) on the parallel region
//   TODO D: pi_local_array - each thread counts into local[id].value (see
//           the comment above pi_local_array); after the region, add up the first
//           `team` entries (record the team size with single).
// The test checks that every version is close to pi and that they agree.
//
// Test:  make test M=04 T=lab2_montecarlo_pi
// Run:   make run  M=04 EX=lab2_montecarlo_pi ARGS="10000000"   (the lab's tables)
#include <omp.h>
#include <stdio.h>
#include <stdlib.h>

// Given: one dart. Returns 1 if (x, y) lands inside the quarter circle.
static int dart(unsigned *seed) {
    double x = (double)rand_r(seed) / RAND_MAX, y = (double)rand_r(seed) / RAND_MAX;
    return x * x + y * y <= 1.0;
}

double pi_serial(long darts) {
    unsigned seed = 42;
    long count = 0;
    for (long i = 0; i < darts; i++) count += dart(&seed);
    return 4.0 * count / darts;
}

double pi_critical(long darts, int nthreads) {
    // TODO
    (void)darts; (void)nthreads; (void)dart;
    return 0.0;
}

double pi_atomic(long darts, int nthreads) {
    // TODO
    (void)darts; (void)nthreads;
    return 0.0;
}

double pi_reduction(long darts, int nthreads) {
    // TODO
    (void)darts; (void)nthreads;
    return 0.0;
}

// For D: one counter per thread. Each sits in its own 64-byte block, so two
// threads never write the same cache line ([EXTRA] false sharing, README 8.6).
// Declare `struct padded local[256] = {{0}};` inside pi_local_array.
struct padded { long value; char pad[64 - sizeof(long)]; };

double pi_local_array(long darts, int nthreads) {
    // TODO
    (void)darts; (void)nthreads; (void)sizeof(struct padded);
    return 0.0;
}

#ifndef TESTING
int main(int argc, char *argv[]) {
    long darts = argc > 1 ? atol(argv[1]) : 10000000;
    const char *name[] = {"A critical", "B atomic", "C reduction", "D local array"};
    double (*fn[])(long, int) = {pi_critical, pi_atomic, pi_reduction, pi_local_array};
    printf("%ld darts, 4 threads\n\n| approach | pi | error | time (s) |\n|---|--:|--:|--:|\n", darts);
    for (int k = 0; k < 4; k++) {
        double t0 = omp_get_wtime(), pi = fn[k](darts, 4), t = omp_get_wtime() - t0;
        printf("| %s | %.6f | %+.6f | %.4f |\n", name[k], pi, pi - 3.14159265358979, t);
    }
    printf("\nreduction with 1, 2, 4, 8 threads\n\n| threads | time (s) | speedup |\n|--:|--:|--:|\n");
    double t1 = 0;
    for (int p = 1; p <= 8; p *= 2) {
        double t0 = omp_get_wtime();
        pi_reduction(darts, p);
        double t = omp_get_wtime() - t0;
        if (p == 1) t1 = t;
        printf("| %d | %.4f | %.2f |\n", p, t, t1 / t);
    }
    return 0;
}
#endif
