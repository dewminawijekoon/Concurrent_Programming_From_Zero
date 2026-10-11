// ANSWER - Exercise 5 (from memory): threads + struct arguments + a mutex-protected sum
//
// Test:  make verify-answers M=03
#include <pthread.h>
#include <stdlib.h>

struct sum_arg {
    const long *a;
    long first, last;            // this thread adds a[first] .. a[last-1]
};

static long total;                                   // shared result
static pthread_mutex_t total_lock = PTHREAD_MUTEX_INITIALIZER;

static void *sum_worker(void *p) {
    struct sum_arg *arg = p;
    long mine = 0;                                   // private partial sum
    for (long i = arg->first; i < arg->last; i++) mine += arg->a[i];
    pthread_mutex_lock(&total_lock);                 // one short critical section
    total += mine;
    pthread_mutex_unlock(&total_lock);
    return NULL;
}

long threaded_sum(const long *a, long n, int nthreads) {
    pthread_t *tid = malloc(sizeof *tid * (size_t)nthreads);
    struct sum_arg *args = malloc(sizeof *args * (size_t)nthreads);
    total = 0;
    for (int t = 0; t < nthreads; t++) {
        args[t].a = a;
        args[t].first = (long)t * n / nthreads;
        args[t].last = (long)(t + 1) * n / nthreads;
        pthread_create(&tid[t], NULL, sum_worker, &args[t]);
    }
    for (int t = 0; t < nthreads; t++) pthread_join(tid[t], NULL);
    free(tid);
    free(args);
    return total;                                    // safe: every thread has finished
}
