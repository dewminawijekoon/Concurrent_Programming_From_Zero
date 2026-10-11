// Interface for lab1_amdahl_measure.c (shared by the exercise and its test)
#ifndef LAB1_AMDAHL_MEASURE_H
#define LAB1_AMDAHL_MEASURE_H

#define LAB_MAX_THREADS 64

void chunk_bounds(long n, int nthreads, int t, long *lo, long *hi);   // given
double amdahl_predict(double p, double n);                            // given
long long parallel_chunk_sum(const int *a, long n, int nthreads);     // Part A
double estimate_p_from_phases(double t_serial, double t_parallel_1);  // Part B
double estimate_p_from_speedup(double s, int n);                      // Part C
#endif
