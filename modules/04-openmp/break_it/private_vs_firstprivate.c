// break_it/private_vs_firstprivate.c - what value does a private copy start with?
// BUILD: -fopenmp
// (Lab 03, Ex 1 Task B)
//
// Run:   make run M=04 EX=private_vs_firstprivate
//
// private(x):      each thread gets a NEW x with NO starting value. Reading
//                  it before writing it is a bug: you may see 0, or garbage.
//                  gcc even warns "'x' is used uninitialized" when compiling this.
// firstprivate(x): each thread gets a new x that starts as a COPY of the outside x.
// In both cases, the outside x is not changed by the region.
#include <omp.h>
#include <stdio.h>

int main(void) {
    int x = 100;
    printf("before: x = %d\n\n", x);
#pragma omp parallel num_threads(4) private(x)
    {
        // BUG on purpose: x has no value yet. Do not do this in real code.
        printf("private(x):      thread %d starts with x = %d\n", omp_get_thread_num(), x);
        x = omp_get_thread_num();
    }
    printf("after private:      x = %d (unchanged)\n\n", x);
#pragma omp parallel num_threads(4) firstprivate(x)
    {
        printf("firstprivate(x): thread %d starts with x = %d\n", omp_get_thread_num(), x);
        x += omp_get_thread_num();
    }
    printf("after firstprivate: x = %d (unchanged)\n", x);
    return 0;
}
