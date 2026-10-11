// Lab track 1 - OpenMP warm-up                                   [Lab 03, Ex 1; L04 slides 45-50]
// BUILD: -fopenmp
//
// PREDICT FIRST: 20 iterations, 4 threads, schedule(static). Which indices
//                does thread 2 compute? With 10 iterations and 4 threads?
//                Will the answer change from run to run?
//
// TODO A (Task A, hello): hello_team(nthreads, seen)
//   Open a parallel region with num_threads(nthreads). Each thread sets
//   seen[its id] = 1 and prints "hello from thread i of p". Return the team
//   size, recorded by ONE thread (#pragma omp single) inside the region.
//
// TODO B (Task B, shared vs private): firstprivate_demo(x, nthreads, start_value, after)
//   Open a region with firstprivate(x). Each thread stores the value its
//   copy of x has at the start in start_value[id], then adds id to its copy.
//   After the region, store x in *after. (Then think: what would
//   start_value be with private(x) instead? See break_it/private_vs_firstprivate.c.)
//
// TODO C (Task C, parallel for): squares_static(arr, owner, n, nthreads)
//   "#pragma omp parallel for schedule(static)" with num_threads(nthreads):
//   arr[i] = i * i and owner[i] = the thread that computed index i.
//
// Test:  make test M=04 T=lab1_warmup
// Run:   make run  M=04 EX=lab1_warmup ARGS="4"
#include <omp.h>
#include <stdio.h>
#include <stdlib.h>

int hello_team(int nthreads, int *seen) {
    // TODO A
    (void)nthreads; (void)seen;
    return 0;
}

void firstprivate_demo(int x, int nthreads, int *start_value, int *after) {
    // TODO B
    (void)x; (void)nthreads; (void)start_value;
    *after = -1;
}

void squares_static(int *arr, int *owner, int n, int nthreads) {
    // TODO C
    (void)arr; (void)owner; (void)n; (void)nthreads;
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
