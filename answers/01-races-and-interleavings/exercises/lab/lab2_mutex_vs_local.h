// Interface for lab2_mutex_vs_local.c (shared by the exercise and its test)
#ifndef LAB2_MUTEX_VS_LOCAL_H
#define LAB2_MUTEX_VS_LOCAL_H
#include <stdatomic.h>

#define LAB2_MAX_THREADS 64

long count_global_mutex(int nthreads, long m);    // Part A: lock for every increment
long count_local_then_add(int nthreads, long m);  // Part B: local sum, one lock per thread
long lock_acquisitions(void);                      // how many times counted_lock() ran
void reset_lock_acquisitions(void);
#endif
