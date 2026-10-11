// break_it/pi_race.c - two threads add to a shared sum with no protection.
//                                                                 [L04 slides 18-20]
// Run:   make run M=03 EX=pi_race
//
// L04 slide 20 reports that with one thread the estimate of pi gets better as
// n grows, but with two threads the answer changes from run to run. This
// program repeats that experiment. `sum += term` is really three steps (read
// sum, add, write sum back). If both threads read the same old value, one
// thread's addition is lost.
#include <pthread.h>
#include <stdio.h>

#define N 20000000L

static volatile double sum;    // volatile: really read and write memory on every step,
                               // as the slide's code does at -O0
static int nthreads;

static void *worker(void *arg) {
    long id = (long)arg;
    long first = id * N / nthreads, last = (id + 1) * N / nthreads;
    double factor = (first % 2 == 0) ? 1.0 : -1.0;
    for (long k = first; k < last; k++, factor = -factor)
        sum += factor / (2 * k + 1);  // RACE: read, add, write are separate steps
    return NULL;
}

static double estimate(int p) {
    pthread_t t[2];
    sum = 0.0;
    nthreads = p;
    for (long i = 0; i < p; i++) pthread_create(&t[i], NULL, worker, (void *)i);
    for (int i = 0; i < p; i++) pthread_join(t[i], NULL);
    return 4.0 * sum;
}

int main(void) {
    printf("n = %ld terms\n", N);
    for (int p = 1; p <= 2; p++)
        for (int run = 1; run <= 3; run++)
            printf("%d thread%s, run %d: pi ~ %.12f\n", p, p == 1 ? " " : "s", run, estimate(p));
    printf("\nWith 1 thread the three runs agree. With 2 threads they usually differ,\n"
           "because some additions are lost. Fix: ex3_pi.c (mutex or local sums).\n");
    return 0;
}
