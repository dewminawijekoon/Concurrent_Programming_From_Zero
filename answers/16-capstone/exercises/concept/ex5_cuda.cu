// Exercise 5 (core) - CUDA: thousands of threads testing numbers   [L07; module 12]
// REQUIRES: cuda
//
// PREDICT FIRST: neighbouring GPU threads test neighbouring numbers p and
//                p + 1. One of them is even (rejected at once) and the other
//                may be a prime (hundreds of divisions). What does that do to
//                a warp of 32 threads that run in lock-step?
//
// Count the twin pairs on the GPU. One kernel, a grid-stride loop over the
// items p = 0 .. N-2 (so a fixed grid handles any N), each thread keeps a
// private count, then adds it to the global total with ONE atomicAdd.
// The CPU reference and the check are given.
//
// Test:  make test M=16 T=ex5_cuda
// Run:   make run  M=16 EX=ex5_cuda ARGS=10000000
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

// The same trial division as twins.h, compiled for both CPU and GPU.
__host__ __device__ int is_prime(long n) {
    if (n < 2) return 0;
    if (n % 2 == 0) return n == 2;
    for (long d = 3; d * d <= n; d += 2)
        if (n % d == 0) return 0;
    return 1;
}

__global__ void twins_kernel(long N, unsigned long long *total) {
    long first = (long)blockIdx.x * blockDim.x + threadIdx.x;
    long step = (long)gridDim.x * blockDim.x;           // all threads of the grid
    unsigned long long mine = 0;                        // private: no race
    for (long p = first; p <= N - 2; p += step)
        if (is_prime(p) && is_prime(p + 2)) mine++;
    if (mine > 0) atomicAdd(total, mine);               // one atomic per thread at most
}

// ---------------------------------------------------------------- given
int main(int argc, char **argv) {
    long N = argc > 1 ? atol(argv[1]) : 1000000;
    long expected = 0;
    for (long p = 0; p <= N - 2; p++) expected += is_prime(p) && is_prime(p + 2);

    unsigned long long *d_total = NULL, total = 0;
    CUDA_CHECK(cudaMalloc((void **)&d_total, sizeof total));
    CUDA_CHECK(cudaMemset(d_total, 0, sizeof total));
    int threads = 256, blocks = 128;                    // a fixed grid; the loop covers the rest
    twins_kernel<<<blocks, threads>>>(N, d_total);
    CUDA_CHECK(cudaGetLastError());
    CUDA_CHECK(cudaDeviceSynchronize());
    CUDA_CHECK(cudaMemcpy(&total, d_total, sizeof total, cudaMemcpyDeviceToHost));
    CUDA_CHECK(cudaFree(d_total));

    printf("N = %ld: GPU count = %llu, CPU count = %ld\n", N, total, expected);
    printf("%s\n", (long)total == expected ? "RESULT: OK" : "RESULT: WRONG");
    return (long)total == expected ? 0 : 1;
}
