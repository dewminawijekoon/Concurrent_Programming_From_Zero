// Interface for ex4_pfib.c (shared by the exercise and its test)
#ifndef EX4_PFIB_H
#define EX4_PFIB_H
#include <stdatomic.h>

long pfib_work(int n);            // strands in the DAG of P-FIB(n)
long pfib_span(int n);            // strands on its longest path
double pfib_parallelism(int n);   // work / span

// fib(n) with a thread per spawn above the cutoff; counts pthread_create calls in *spawns.
long pfib_threads(int n, int cutoff, atomic_long *spawns);
#endif
