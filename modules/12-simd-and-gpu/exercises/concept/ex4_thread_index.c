// Exercise 4 (core) - Which element does each GPU thread handle?   [L07 slides 19-21, 29-31]
//
// PREDICT FIRST: you launch  kernel<<<3, 4>>>  on an array of 10 elements, and
//                each thread computes i = blockIdx.x * blockDim.x + threadIdx.x.
//                Which (block, thread) handles element 9? How many threads
//                find that their i is outside the array?
//
// You do not need a GPU for this one. Plain C functions play the role of the
// CUDA built-in variables (blockIdx.x, blockDim.x, threadIdx.x, gridDim.x),
// so you can check your index arithmetic on any machine. Getting this
// arithmetic right is half of every CUDA exam answer.
//
// Test:  make test M=12 T=ex4_thread_index
// Run:   make run  M=12 EX=ex4_thread_index
#include <stdio.h>

// The global index of a thread in a 1-D launch: how many threads come before
// it in the earlier blocks, plus its position inside its own block.
int global_index(int block_idx, int block_dim, int thread_idx) {
    // TODO
    (void)block_idx; (void)block_dim; (void)thread_idx;
    return 0;
}

// How many blocks of `threads_per_block` are needed to cover n elements?
// Round UP: 1000 elements with 256 threads per block need 4 blocks, not 3.
int blocks_needed(int n, int threads_per_block) {
    // TODO (integer division rounds DOWN, so add something first)
    (void)n; (void)threads_per_block;
    return 0;
}

// Pretend to run   kernel<<<blocks, threads>>>(n)   where every thread does
//     int i = blockIdx.x * blockDim.x + threadIdx.x;
//     if (i < n) { ...handle element i... }
// For each element e, record which block and which thread handled it
// (owner_block[e], owner_thread[e]). The caller fills both arrays with -1
// first, so any element nobody handled stays -1.
// Return the number of threads that did nothing because their i >= n.
int simulate_1d_launch(int n, int blocks, int threads, int *owner_block, int *owner_thread) {
    // TODO: two loops (over blocks, then over threads in a block) play the
    //       role of the GPU running all threads. Use global_index().
    (void)n; (void)blocks; (void)threads; (void)owner_block; (void)owner_thread;
    return 0;
}

// A "grid-stride loop" lets a fixed number of threads handle any n:
//     for (int i = global id; i < n; i += total number of threads) ...
// How many elements does the thread with global id g handle,
// if total_threads threads run this loop over n elements?
int grid_stride_count(int n, int total_threads, int g) {
    // TODO (a loop is fine; a formula is nicer)
    (void)n; (void)total_threads; (void)g;
    return 0;
}

// The matrix kernel of L07 slides 30-31 uses a 2-D grid of 1-D blocks:
//     column = blockIdx.x * blockDim.x + threadIdx.x
//     row    = blockIdx.y
// Fill *row and *col for the thread (bx, by, tx) with blocks of block_dim_x threads.
void matmul_cell(int bx, int by, int tx, int block_dim_x, int *row, int *col) {
    // TODO
    (void)bx; (void)by; (void)tx; (void)block_dim_x;
    *row = -1;
    *col = -1;
}

#ifndef TESTING
int main(void) {
    int n = 10, ob[10], ot[10];
    for (int e = 0; e < n; e++) ob[e] = ot[e] = -1;
    int idle = simulate_1d_launch(n, 3, 4, ob, ot);
    printf("kernel<<<3, 4>>> on %d elements\n", n);
    printf("element | block | thread\n");
    for (int e = 0; e < n; e++) printf("%7d | %5d | %6d\n", e, ob[e], ot[e]);
    printf("idle threads: %d\n", idle);
    printf("blocks needed for 1000 elements, 256 per block: %d\n", blocks_needed(1000, 256));
    int r, c;
    matmul_cell(2, 5, 7, 32, &r, &c);
    printf("matrix kernel, block (2,5), thread 7, 32 threads per block -> row %d, column %d\n", r, c);
    return 0;
}
#endif
