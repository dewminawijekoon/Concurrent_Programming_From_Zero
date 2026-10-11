// Interface for ex2_split_the_work.c (shared by the exercise and its test)
#ifndef EX2_SPLIT_THE_WORK_H
#define EX2_SPLIT_THE_WORK_H

#define SPLIT_MAX_THREADS 64

struct thread_stats {
    long primes;     // primes found by this thread
    long divisions;  // trial divisions done by this thread (its amount of work)
};

// Given: is x prime? Adds the number of trial divisions tried to *divisions.
int is_prime_counted(long x, long *divisions);

// Thread t of nthreads gets the block [*my_lo, *my_hi) of [lo, hi).
void split_range(long lo, long hi, int nthreads, int t, long *my_lo, long *my_hi);

// Count primes in 1..limit with nthreads threads (static split).
// Fills per_thread[0..nthreads-1]; returns the total, or -1 for a bad nthreads.
long count_primes_static(long limit, int nthreads, struct thread_stats per_thread[]);
#endif
