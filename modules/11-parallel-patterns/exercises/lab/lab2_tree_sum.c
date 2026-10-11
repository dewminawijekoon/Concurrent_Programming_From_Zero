// Lab track 2 - divide & conquer: parallel sum of an unbalanced tree
//                                [Lab 06, Ex 2; L02 slides 14-19 (P-FIB, spawn/sync)]
//
// PREDICT FIRST: you start a new thread for EVERY node of a tree with a
//                million nodes. Is that faster or slower than one thread?
//                Now you start threads only for subtrees bigger than 10 000
//                nodes. Why is that better? And why is 1 000 000 bad too?
//
// Divide & conquer: split the problem into parts, solve the parts at the same
// time, then combine the answers. L02's P-FIB does this with "spawn" (start a
// child) and "sync" (wait for it). With pthreads:
//     spawn  =  pthread_create(... left subtree ...)
//     (meanwhile do the right subtree in THIS thread)
//     sync   =  pthread_join
// Starting a thread costs tens of microseconds, so for small subtrees it is
// cheaper to just add them up serially. The size where we stop splitting is
// called the THRESHOLD (or cutoff).
//
// Your task: tree_sum_parallel(n, threshold)
//   - if n is NULL, return 0
//   - if the subtree has at most `threshold` nodes (n->size), return
//     tree_sum_serial(n)
//   - otherwise start a thread for the left subtree (call
//     atomic_fetch_add(&threads_spawned, 1) when pthread_create succeeds),
//     sum the right subtree in this thread, join, and return
//     n->value + left sum + right sum.
//   - if pthread_create fails (too many threads), sum the left part yourself.
//
// Test:  make test M=11 T=lab2_tree_sum
// Run:   make run  M=11 EX=lab2_tree_sum ARGS="38 200"  (tree levels: 38 = about 2.7 million
//                                                     nodes; work per node)
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>

#include "bench.h"

typedef struct tnode {
    long value;
    long size;               // number of nodes in this subtree (this node included)
    struct tnode *left, *right;
} tnode;

atomic_long threads_spawned;   // how many threads tree_sum_parallel started

// Given: an unbalanced tree. The left subtree has one level less, the right
// subtree three levels less, so left subtrees are much bigger than right ones.
// Values are 1, 2, 3, ... so the sum of a tree with N nodes is N(N+1)/2.
tnode *tree_build(int levels, long *next_value) {
    if (levels <= 0) return NULL;
    tnode *n = malloc(sizeof *n);
    n->value = ++*next_value;
    n->left = tree_build(levels - 1, next_value);
    n->right = tree_build(levels - 3, next_value);
    n->size = 1 + (n->left ? n->left->size : 0) + (n->right ? n->right->size : 0);
    return n;
}

void tree_free(tnode *n) {
    if (!n) return;
    tree_free(n->left);
    tree_free(n->right);
    free(n);
}

// Given: pretend that visiting a node is real work (like hashing a record),
// so that there is something worth doing in parallel. Returns the node's value.
int node_work = 200;
static long visit(const tnode *n) {
    volatile unsigned x = (unsigned)n->value;
    for (int k = 0; k < node_work; k++) x = x * 1664525u + 1013904223u;
    return n->value;
}

// Given: the serial sum.
long tree_sum_serial(const tnode *n) {
    if (!n) return 0;
    return visit(n) + tree_sum_serial(n->left) + tree_sum_serial(n->right);
}

long tree_sum_parallel(const tnode *n, long threshold);

// What a child thread needs: its subtree in, its sum out.
typedef struct {
    const tnode *root;
    long threshold;
    long sum;
} sum_job;

static void *sum_job_main(void *arg) {
    sum_job *j = arg;
    // TODO: j->sum = the parallel sum of j->root (same threshold)
    (void)j;
    return NULL;
}

long tree_sum_parallel(const tnode *n, long threshold) {
    // TODO: follow the four steps at the top of the file.
    //       The child's sum_job must stay alive until you join it
    //       (a local variable in this function is fine).
    (void)n; (void)threshold; (void)sum_job_main;
    return 0;
}

#ifndef TESTING
int main(int argc, char **argv) {
    int levels = argc > 1 ? atoi(argv[1]) : 38;
    if (argc > 2) node_work = atoi(argv[2]);
    long next = 0;
    tnode *root = tree_build(levels, &next);
    printf("tree with %d levels: %ld nodes\n", levels, root->size);

    double samples[16];
    int reps = bench_reps(3);
    if (reps > 16) reps = 16;
    long s = 0;
    for (int r = 0; r < reps; r++) {
        double t0 = bench_now();
        s = tree_sum_serial(root);
        samples[r] = bench_now() - t0;
    }
    double ts = bench_median(samples, reps);
    printf("serial sum = %ld in %.4f s (median of %d)\n", s, ts, reps);

    long thresholds[] = {100, 1000, 10000, 100000, 1000000};
    printf("\n| threshold | threads started | time (s) | speedup | correct? |\n");
    printf("|----------:|----------------:|---------:|--------:|:--------:|\n");
    for (int k = 0; k < 5; k++) {
        long ps = 0;
        for (int r = 0; r < reps; r++) {
            atomic_store(&threads_spawned, 0);
            double t0 = bench_now();
            ps = tree_sum_parallel(root, thresholds[k]);
            samples[r] = bench_now() - t0;
        }
        double tp = bench_median(samples, reps);
        printf("| %9ld | %15ld | %8.4f | %7.2f | %8s |\n", thresholds[k],
               atomic_load(&threads_spawned), tp, ts / tp, ps == s ? "yes" : "NO");
    }
    tree_free(root);
    return 0;
}
#endif
