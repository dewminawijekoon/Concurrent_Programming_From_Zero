// ANSWER - Lab track 1: OpenMP warm-up                          [Lab 03, Ex 1; L04 slides 45-50]
// BUILD: -fopenmp
//
// Test:  make verify-answers M=04
// Run:   make run M=04 EX=lab1_warmup ANS=1 ARGS="4"
#include <omp.h>
#include <stdio.h>
#include <stdlib.h>

// Task A
int hello_team(int nthreads, int *seen) {
    int team = 0;
#pragma omp parallel num_threads(nthreads)
    {
        int id = omp_get_thread_num();          // private: declared inside the region
        seen[id] = 1;                           // each thread writes its own slot
        printf("hello from thread %d of %d\n", id, omp_get_num_threads());
#pragma omp single
        team = omp_get_num_threads();           // one thread records the team size
    }
    return team;
}

// Task B
void firstprivate_demo(int x, int nthreads, int *start_value, int *after) {
#pragma omp parallel num_threads(nthreads) firstprivate(x)
    {
        int id = omp_get_thread_num();
        start_value[id] = x;                    // every copy starts as the outside x
        x += id;                                // changes only this thread's copy
    }
    *after = x;                                 // the original x was never touched
}

// Task C
void squares_static(int *arr, int *owner, int n, int nthreads) {
#pragma omp parallel for schedule(static) num_threads(nthreads)
    for (int i = 0; i < n; i++) {
        arr[i] = i * i;
        owner[i] = omp_get_thread_num();
    }
}

#ifndef TESTING
int main(int argc, char *argv[]) {
    int p = argc > 1 ? atoi(argv[1]) : 4;
    int seen[256] = {0}, start[256], after, arr[20], owner[20];
    printf("Task A: team of %d\n\n", hello_team(p, seen));
    firstprivate_demo(100, p, start, &after);
    printf("Task B: every thread started with x = %d; after the region x = %d\n\n", start[0], after);
    squares_static(arr, owner, 20, p);
    printf("Task C: schedule(static), 20 iterations, %d threads\n", p);
    for (int i = 0; i < 20; i++) printf("  arr[%2d] = %3d  computed by thread %d\n", i, arr[i], owner[i]);
    return 0;
}
#endif
