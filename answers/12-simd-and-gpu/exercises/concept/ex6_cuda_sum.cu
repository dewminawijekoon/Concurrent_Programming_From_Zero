// Exercise 6 (core) - Summing an array on the GPU: race, atomics, shared memory
//                                     [L07 slides 36, 41, 42; README sections 3.7 and 8.4]
// REQUIRES: cuda
//
// PREDICT FIRST: a kernel where every thread does  *total += x[i];  runs on
//                a million elements. Is the answer right? Bigger, smaller, or
//                random? (Module 01 asked the same question for two threads.)
//
// The program runs three kernels on the same array and checks each answer:
//   sum_racy   (given)  *total += x[i]       -> loses updates: a data race
//   sum_atomic (you)    atomicAdd per thread -> correct, but every thread
//                                               queues for the same address
//   sum_shared (you)    each block adds its part in fast shared memory, then
//                       thread 0 of the block does ONE atomicAdd
// Only sum_atomic and sum_shared are checked. sum_racy is there so you can
// see the lost updates on a real GPU.
//
// Test:  make test M=12 T=ex6_cuda_sum
// Run:   make run  M=12 EX=ex6_cuda_sum
#include <stdio.h>
#include <stdlib.h>

#define CUDA_CHECK(call)                                                        \
    do {                                                                        \
        cudaError_t err_ = (call);                                              \
        if (err_ != cudaSuccess) {                                              \
            fprintf(stderr, "CUDA error at %s:%d: %s\n", __FILE__, __LINE__,    \
                    cudaGetErrorString(err_));                                  \
            exit(1);                                                            \
        }                                                                       \
    } while (0)

#define BLOCK 256   // threads per block; must be a power of two for sum_shared

// ---------------------------------------------------------------- given (broken on purpose)
__global__ void sum_racy(const int *x, int n, unsigned long long *total) {
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i < n) *total += x[i];          // read, add, write: other threads can slip in between
}

// ---------------------------------------------------------------- your code
// Every thread adds its own element to *total with one atomic operation.
__global__ void sum_atomic(const int *x, int n, unsigned long long *total) {
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i < n) atomicAdd(total, (unsigned long long)x[i]);   // read-add-write as one step
}

// Tree reduction inside each block (README section 8.4), then one atomicAdd per block.
__global__ void sum_shared(const int *x, int n, unsigned long long *total) {
    __shared__ unsigned long long part[BLOCK];    // one slot per thread of this block
    int t = threadIdx.x;
    int i = blockIdx.x * blockDim.x + t;
    part[t] = (i < n) ? (unsigned long long)x[i] : 0;   // threads past the end add 0
    __syncthreads();                              // all slots are filled before anyone reads them

    for (int stride = blockDim.x / 2; stride > 0; stride /= 2) {
        if (t < stride) part[t] += part[t + stride];    // left half adds in the right half
        __syncthreads();                          // finish this round before the next one starts
    }
    if (t == 0) atomicAdd(total, part[0]);        // one atomic per block instead of per thread
}

// ---------------------------------------------------------------- given
enum { RACY, ATOMIC, SHARED };

static unsigned long long run(int which, const int *d_x, int n, float *ms) {
    unsigned long long *d_total = NULL, total = 0;
    CUDA_CHECK(cudaMalloc((void **)&d_total, sizeof *d_total));
    CUDA_CHECK(cudaMemset(d_total, 0, sizeof *d_total));
    cudaEvent_t start, stop;
    CUDA_CHECK(cudaEventCreate(&start));
    CUDA_CHECK(cudaEventCreate(&stop));
    int blocks = (n + BLOCK - 1) / BLOCK;
    CUDA_CHECK(cudaEventRecord(start));
    if (which == RACY) sum_racy<<<blocks, BLOCK>>>(d_x, n, d_total);
    if (which == ATOMIC) sum_atomic<<<blocks, BLOCK>>>(d_x, n, d_total);
    if (which == SHARED) sum_shared<<<blocks, BLOCK>>>(d_x, n, d_total);
    CUDA_CHECK(cudaGetLastError());
    CUDA_CHECK(cudaEventRecord(stop));
    CUDA_CHECK(cudaEventSynchronize(stop));
    CUDA_CHECK(cudaEventElapsedTime(ms, start, stop));
    CUDA_CHECK(cudaMemcpy(&total, d_total, sizeof total, cudaMemcpyDeviceToHost));
    CUDA_CHECK(cudaFree(d_total));
    CUDA_CHECK(cudaEventDestroy(start));
    CUDA_CHECK(cudaEventDestroy(stop));
    return total;
}

int main(int argc, char **argv) {
    int n = argc > 1 ? atoi(argv[1]) : (1 << 20) + 123;
    int *x = (int *)malloc((size_t)n * sizeof(int));
    unsigned long long expected = 0;
    for (int i = 0; i < n; i++) { x[i] = i % 10; expected += (unsigned long long)x[i]; }

    int *d_x = NULL;
    CUDA_CHECK(cudaMalloc((void **)&d_x, (size_t)n * sizeof(int)));
    CUDA_CHECK(cudaMemcpy(d_x, x, (size_t)n * sizeof(int), cudaMemcpyHostToDevice));

    float ms;
    unsigned long long r = run(RACY, d_x, n, &ms);
    printf("racy:   %llu (expected %llu) %s  [%.3f ms]\n", r, expected,
           r == expected ? "- lucky this time" : "- updates were lost", ms);
    int ok = 1;
    r = run(ATOMIC, d_x, n, &ms);
    printf("atomic: %s (got %llu, expected %llu)  [%.3f ms]\n", r == expected ? "OK" : "WRONG", r, expected, ms);
    ok &= r == expected;
    r = run(SHARED, d_x, n, &ms);
    printf("shared: %s (got %llu, expected %llu)  [%.3f ms]\n", r == expected ? "OK" : "WRONG", r, expected, ms);
    ok &= r == expected;

    CUDA_CHECK(cudaFree(d_x));
    free(x);
    return ok ? 0 : 1;
}
