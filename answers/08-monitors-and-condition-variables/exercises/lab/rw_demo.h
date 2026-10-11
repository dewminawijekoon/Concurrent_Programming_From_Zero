// rw_demo.h - the Lab 02 Ex 4/5 style demo for lab tracks 4 and 5 (given; no TODOs).
//
// R readers and W writers each do 5 rounds. Every start and finish is logged
// with a timestamp, like the lab asks. At the end it prints how long writers
// waited on average to get in (Lab 02 Q5.1), and the largest number of readers
// that were inside together (Lab 02 Q4.1).
#ifndef RW_DEMO_H
#define RW_DEMO_H

#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "bench.h"

void rw_init(void);
void start_read(void);
void end_read(void);
void start_write(void);
void end_write(void);

static int shared_value = 0;          // the data: written only by writers
static double demo_t0;
static atomic_int demo_readers_inside, demo_max_readers;
static double demo_write_wait_total;  // protected by demo_mu
static int demo_writes;
static pthread_mutex_t demo_mu = PTHREAD_MUTEX_INITIALIZER;

static double demo_elapsed(void) { return bench_now() - demo_t0; }

static void demo_pause(unsigned *seed, int max_us) { usleep((useconds_t)(rand_r(seed) % max_us)); }

static void *demo_reader(void *arg) {
    int id = (int)(long)arg;
    unsigned seed = 1234u + (unsigned)id;
    for (int i = 0; i < 5; i++) {
        start_read();
        int now = atomic_fetch_add(&demo_readers_inside, 1) + 1;
        int old = atomic_load(&demo_max_readers);
        while (now > old && !atomic_compare_exchange_weak(&demo_max_readers, &old, now)) {
        }
        printf("[%7.3fs] Reader %d: started reading (value=%d, active_readers=%d)\n",
               demo_elapsed(), id, shared_value, now);
        demo_pause(&seed, 100000);                      // simulate reading
        printf("[%7.3fs] Reader %d: finished reading\n", demo_elapsed(), id);
        atomic_fetch_sub(&demo_readers_inside, 1);
        end_read();
        demo_pause(&seed, 50000);                       // think time
    }
    return NULL;
}

static void *demo_writer(void *arg) {
    int id = (int)(long)arg;
    unsigned seed = 999u + (unsigned)id;
    for (int i = 0; i < 5; i++) {
        double asked = demo_elapsed();
        start_write();
        double waited = demo_elapsed() - asked;
        int old = shared_value;
        shared_value++;
        printf("[%7.3fs] Writer %d: started writing (value %d -> %d, active_readers=%d, waited %.3fs)\n",
               demo_elapsed(), id, old, shared_value, atomic_load(&demo_readers_inside), waited);
        demo_pause(&seed, 100000);                      // simulate writing
        printf("[%7.3fs] Writer %d: finished writing\n", demo_elapsed(), id);
        end_write();
        pthread_mutex_lock(&demo_mu);
        demo_write_wait_total += waited;
        demo_writes++;
        pthread_mutex_unlock(&demo_mu);
        demo_pause(&seed, 50000);                       // think time
    }
    return NULL;
}

static int rw_demo(int argc, char **argv, const char *name) {
    int R = argc > 1 ? atoi(argv[1]) : 5;
    int W = argc > 2 ? atoi(argv[2]) : 5;
    if (R < 0 || W < 0 || R + W > 64) { printf("usage: R W  (R + W <= 64)\n"); return 1; }
    printf("Reader-writer (%s): R=%d readers, W=%d writers, 5 rounds each\n", name, R, W);
    rw_init();
    pthread_t t[64];
    demo_t0 = bench_now();
    for (long i = 0; i < R; i++) pthread_create(&t[i], NULL, demo_reader, (void *)(i + 1));
    for (long i = 0; i < W; i++) pthread_create(&t[R + i], NULL, demo_writer, (void *)(i + 1));
    for (int i = 0; i < R + W; i++) pthread_join(t[i], NULL);
    printf("\nfinal value = %d (expected %d)\n", shared_value, 5 * W);
    printf("most readers inside at the same time: %d\n", atomic_load(&demo_max_readers));
    if (demo_writes)
        printf("average time a writer waited to get in: %.3f s (over %d writes); total run %.3f s\n",
               demo_write_wait_total / demo_writes, demo_writes, demo_elapsed());
    return 0;
}

#endif
