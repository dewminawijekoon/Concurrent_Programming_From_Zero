// ANSWER - Lab track 1: create N threads, collect what each one returns   [Lab 01, Ex 1]
//
// Test:  make verify-answers M=03
// Run:   make run M=03 EX=lab1_create_join ANS=1 ARGS="8"
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

void *square_worker(void *arg) {
    // The index was stored IN the pointer value itself (not pointed to), so
    // we convert the pointer back to an integer. intptr_t is an integer type
    // exactly big enough to hold a pointer, so nothing is lost either way.
    long index = (long)(intptr_t)arg;
    printf("thread with index %2ld (pthread id %lu) returns %ld\n", index,
           (unsigned long)pthread_self(), index * index);
    // Return the result the same way: an integer stored in a pointer value.
    return (void *)(intptr_t)(index * index);
}

int create_and_join(int n, long *results) {
    pthread_t *tid = malloc(sizeof *tid * (size_t)n);
    if (tid == NULL) return -1;
    for (long i = 0; i < n; i++)
        if (pthread_create(&tid[i], NULL, square_worker, (void *)(intptr_t)i) != 0) return -1;
    for (int i = 0; i < n; i++) {
        void *ret;                       // pthread_join writes the thread's return value here
        pthread_join(tid[i], &ret);
        results[i] = (long)(intptr_t)ret;
    }
    free(tid);
    return 0;
}

#ifndef TESTING
int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "usage: %s <num_threads>\n", argv[0]);
        return 1;
    }
    int n = atoi(argv[1]);
    long *results = malloc(sizeof *results * (size_t)n);
    if (create_and_join(n, results) != 0) return 1;
    printf("main collected:");
    for (int i = 0; i < n; i++) printf(" %ld", results[i]);
    printf("\n");
    free(results);
    return 0;
}
#endif
