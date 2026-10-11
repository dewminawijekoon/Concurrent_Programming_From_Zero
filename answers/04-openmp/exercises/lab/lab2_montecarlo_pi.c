// ANSWER - Lab track 2: Monte Carlo pi, four ways to fix the race       [Lab 03, Ex 2]
// BUILD: -fopenmp
//
// Test:  make verify-answers M=04
// Run:   make run M=04 EX=lab2_montecarlo_pi ANS=1 ARGS="10000000"
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

// A: critical. Correct, but every hit queues for the same lock.
double pi_critical(long darts, int nthreads) {
    long count = 0;
#pragma omp parallel num_threads(nthreads)
    {
        unsigned seed = 42 + omp_get_thread_num();   // private: declared inside
#pragma omp for
        for (long i = 0; i < darts; i++)
            if (dart(&seed)) {
#pragma omp critical
                count++;
            }
    }
    return 4.0 * count / darts;
}

// B: atomic. One hardware read-modify-write per hit: cheaper than critical,
// but all threads still fight over the same memory location.
double pi_atomic(long darts, int nthreads) {
    long count = 0;
#pragma omp parallel num_threads(nthreads)
    {
        unsigned seed = 42 + omp_get_thread_num();
#pragma omp for
        for (long i = 0; i < darts; i++)
            if (dart(&seed)) {
#pragma omp atomic
                count++;
            }
    }
    return 4.0 * count / darts;
}

// C: reduction. Each thread counts in a private copy; OpenMP adds them once.
double pi_reduction(long darts, int nthreads) {
    long count = 0;
#pragma omp parallel num_threads(nthreads) reduction(+ : count)
    {
        unsigned seed = 42 + omp_get_thread_num();
#pragma omp for
        for (long i = 0; i < darts; i++) count += dart(&seed);
    }
    return 4.0 * count / darts;
}

// D: our own per-thread counters, combined after the region. This is what
// reduction does for you. Each counter sits in its own 64-byte block, so two
// threads never write the same cache line ([EXTRA] false sharing, README 8.6).
struct padded { long value; char pad[64 - sizeof(long)]; };

double pi_local_array(long darts, int nthreads) {
    struct padded local[256] = {{0}};
    int team = 1;
#pragma omp parallel num_threads(nthreads)
    {
        int id = omp_get_thread_num();
        unsigned seed = 42 + id;
#pragma omp single
        team = omp_get_num_threads();
#pragma omp for
        for (long i = 0; i < darts; i++) local[id].value += dart(&seed);
    }
    long count = 0;
    for (int t = 0; t < team; t++) count += local[t].value;   // serial, after the join
    return 4.0 * count / darts;
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
