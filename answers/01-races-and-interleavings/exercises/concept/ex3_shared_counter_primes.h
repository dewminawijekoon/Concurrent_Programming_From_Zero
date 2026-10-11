// Interface for ex3_shared_counter_primes.c (shared by the exercise and its test)
#ifndef EX3_SHARED_COUNTER_PRIMES_H
#define EX3_SHARED_COUNTER_PRIMES_H
#include <pthread.h>

#define PRIMES_MAX_THREADS 64

struct counter {
    long value;            // the next number to hand out
    pthread_mutex_t lock;  // protects value
};

struct prime_stats {
    long taken;      // how many numbers this thread got from the counter
    long primes;     // how many of them were prime
    long divisions;  // trial divisions done (amount of work)
};

int is_prime_counted(long x, long *divisions);   // given
void counter_init(struct counter *c, long start);
long get_and_increment(struct counter *c);       // return the old value, add 1: atomically
long count_primes_dynamic(long limit, int nthreads, struct prime_stats per_thread[]);
#endif
