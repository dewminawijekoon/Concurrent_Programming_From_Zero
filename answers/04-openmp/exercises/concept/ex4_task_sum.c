// ANSWER - Exercise 4 (core): divide and conquer with OpenMP tasks       [EXTRA; Lab 03 Ex 4]
// BUILD: -fopenmp
//
// Test:  make verify-answers M=04
// Run:   make run M=04 EX=ex4_task_sum ANS=1 ARGS="1000"
#include <omp.h>
#include <stdio.h>
#include <stdlib.h>

static long tasks_made;               // shared counter, updated with atomic

static long sum_rec(const long *a, long n, long cutoff) {
    if (n <= cutoff) {                // small piece: a task would cost more than it saves
        long s = 0;
        for (long i = 0; i < n; i++) s += a[i];
        return s;
    }
    long half = n / 2, left, right;
    // Each task gets its own copies of a, half, n, cutoff (firstprivate is
    // the default for variables local to this function). left and right
    // must be shared, so the tasks' results are visible here after taskwait.
#pragma omp task shared(left)
    left = sum_rec(a, half, cutoff);
#pragma omp task shared(right)
    right = sum_rec(a + half, n - half, cutoff);
#pragma omp atomic
    tasks_made += 2;
#pragma omp taskwait                  // wait for the two child tasks
    return left + right;
}

long task_sum(const long *a, long n, long cutoff, int nthreads, long *tasks_created) {
    long result = 0;
    tasks_made = 0;
#pragma omp parallel num_threads(nthreads)
    {
        // Every thread of the team runs this block. Without single, EACH
        // thread would start the whole recursion, and we would add the
        // array up nthreads times. single: one thread starts it; the others
        // wait at the end of single and meanwhile run the tasks it creates.
#pragma omp single
        result = sum_rec(a, n, cutoff);
    }
    *tasks_created = tasks_made;
    return result;
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
