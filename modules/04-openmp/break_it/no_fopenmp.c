// break_it/no_fopenmp.c - what happens when you forget -fopenmp       [Lab 03 prerequisites]
//
// Run:   make run M=04 EX=no_fopenmp
//
// This file has NO "BUILD: -fopenmp" line, so it is compiled without OpenMP.
// Watch the compiler output: gcc only WARNS "ignoring #pragma omp parallel"
// (and only because we compile with -Wall). The program still builds and
// runs, but on ONE thread. Without -Wall you would get no message at all.
// The _OPENMP macro is defined only when OpenMP is switched on, so a program
// can check for it.
#include <stdio.h>
#ifdef _OPENMP
#include <omp.h>
#endif

int main(void) {
    int count = 0;
#pragma omp parallel num_threads(4)
    {
#pragma omp atomic
        count++;                          // how many threads ran this block?
    }
#ifdef _OPENMP
    printf("OpenMP is ON: %d threads ran the parallel block\n", count);
#else
    printf("OpenMP is OFF (no -fopenmp): the pragmas were ignored, %d thread ran the block\n", count);
#endif
    return 0;
}
