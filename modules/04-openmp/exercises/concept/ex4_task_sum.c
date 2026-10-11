// Exercise 4 (core) - divide and conquer with OpenMP tasks               [EXTRA; Lab 03 Ex 4]
// BUILD: -fopenmp
//
// Sum an array by splitting it in half again and again, like merge sort.
// This is the warm-up for the parallel merge sort in exercises/lab/.
//
// PREDICT FIRST: with n = 1,000,000 and cutoff = 1, how many tasks are
//                created? With cutoff = 10,000? Which one will be faster?
//
// TODO 1: sum_rec(a, n, cutoff)
//   - if n <= cutoff: add a[0..n-1] with a plain loop and return the sum;
//   - otherwise: half = n / 2. Create one task for the left part
//     (a, half elements) and one for the right part (a + half, n - half
//     elements). Each task stores its result in its own variable, which
//     must be shared(...) with the task. Add 2 to tasks_made with
//     "#pragma omp atomic". Then "#pragma omp taskwait" and return
//     left + right.
// TODO 2: task_sum - open a parallel region with num_threads(nthreads) and
//   make ONE thread call sum_rec ("#pragma omp single"). Store tasks_made
//   in *tasks_created.
// The test counts your tasks, so split exactly as described.
//
// Test:  make test M=04 T=ex4_task_sum
// Run:   make run  M=04 EX=ex4_task_sum ARGS="1000"     (cutoff 1000)
#include <omp.h>
#include <stdio.h>
#include <stdlib.h>

static long tasks_made;               // shared counter, updated with atomic

static long sum_rec(const long *a, long n, long cutoff) {
    // TODO 1
    (void)a; (void)n; (void)cutoff;
    return 0;
}

long task_sum(const long *a, long n, long cutoff, int nthreads, long *tasks_created) {
    // TODO 2
    (void)a; (void)n; (void)cutoff; (void)nthreads; (void)sum_rec;
    tasks_made = 0;
    *tasks_created = tasks_made;
    return 0;
}

#ifndef TESTING
int main(int argc, char *argv[]) {
    long n = 20000000, cutoff = argc > 1 ? atol(argv[1]) : 10000, made;
    long *a = malloc(sizeof *a * (size_t)n);
    for (long i = 0; i < n; i++) a[i] = i % 10;
    for (int p = 1; p <= 8; p *= 2) {
        double t0 = omp_get_wtime();
        long s = task_sum(a, n, cutoff, p, &made);
        printf("%d threads: sum %ld, %ld tasks, %.4f s\n", p, s, made, omp_get_wtime() - t0);
    }
    free(a);
    return 0;
}
#endif
