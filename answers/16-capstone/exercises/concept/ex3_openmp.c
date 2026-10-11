// Exercise 3 (core) - OpenMP: the same problem with schedule clauses   [L04; module 04]
// BUILD: -fopenmp
//
// PREDICT FIRST: which OpenMP schedule matches each Pthreads strategy of
//                exercise 2: block, cyclic, dynamic? Which one would you
//                write in the exam for this problem, and why?
//
// twins_omp(N, T, kind, chunk, work) counts the twin pairs with an OpenMP
// parallel for over the items, using T threads, and the schedule given by
// (kind, chunk): kind is omp_sched_static, omp_sched_dynamic or
// omp_sched_guided; chunk 0 means "OpenMP's default chunk".
// We set the schedule at run time with omp_set_schedule(), so your loop must
// use  schedule(runtime) . Add the counts with a reduction. work[t] must hold
// the work done by thread t: keep a private counter per thread and store it
// once at the end (omp_get_thread_num() tells a thread its number).
//
// Test:  make test M=16 T=ex3_openmp           (prints a work and time table)
// Run:   make run  M=16 EX=ex3_openmp ARGS=2000000
#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include "twins.h"

long twins_omp(long N, int T, omp_sched_t kind, int chunk, long *work) {
    long M = tw_items(N), count = 0;
    omp_set_schedule(kind, chunk);                 // used by schedule(runtime)
    for (int t = 0; t < T; t++) work[t] = 0;
    #pragma omp parallel num_threads(T) reduction(+:count)
    {
        long my_work = 0;                          // private: declared inside the parallel region
        #pragma omp for schedule(runtime)
        for (long p = 0; p < M; p++) count += tw_item(p, &my_work);
        work[omp_get_thread_num()] = my_work;      // each thread writes its own slot
    }
    return count;
}

#ifndef TESTING
int main(int argc, char **argv) {
    long N = argc > 1 ? atol(argv[1]) : 2000000;
    int T = 4;
    long work[64];
    struct { const char *name; omp_sched_t k; int c; } s[] = {
        {"static", omp_sched_static, 0}, {"static,1", omp_sched_static, 1},
        {"dynamic,64", omp_sched_dynamic, 64}, {"guided", omp_sched_guided, 0}};
    for (int i = 0; i < 4; i++) {
        double t0 = omp_get_wtime();
        long c = twins_omp(N, T, s[i].k, s[i].c, work);
        printf("schedule(%-10s) count = %ld, %.3f s\n", s[i].name, c, omp_get_wtime() - t0);
    }
    return 0;
}
#endif
