// Lab track 1 - create N threads, collect what each one returns       [Lab 01, Ex 1]
//
// PREDICT FIRST: run with 8 threads several times. Will the lines always
//                appear in the same order? Will main's list of results
//                always be in the same order? Why is the answer different
//                for the two questions?
//
// create_and_join(n, results) starts n threads. Thread i gets its index i
// through the void *arg of pthread_create. It prints its index and its
// pthread id, and RETURNS i * i. The caller joins every thread and stores
// the value thread i returned in results[i]. Return 0 on success.
//
// Passing a small integer through a void *: store the number in the
// pointer value itself, (void *)(intptr_t)i, and convert it back with
// (long)(intptr_t)arg. This copies the VALUE, so later changes to i do not
// matter. (Do NOT pass &i: see break_it/loop_index_bug.c.)
// Return the square the same way, and read it with the second argument of
// pthread_join(tid, &ret).
//
// Test:  make test M=03 T=lab1_create_join
// Run:   make run  M=03 EX=lab1_create_join ARGS="8"
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

void *square_worker(void *arg) {
    // TODO: get the index back from arg, print it with pthread_self(),
    //       return index * index.
    (void)arg;
    return NULL;
}

int create_and_join(int n, long *results) {
    // TODO: create n threads running square_worker, then join them all and
    //       store each returned value in results[i].
    (void)n; (void)results;
    return -1;
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
