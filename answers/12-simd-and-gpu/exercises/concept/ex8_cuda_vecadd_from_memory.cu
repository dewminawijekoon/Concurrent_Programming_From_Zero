// Exercise 8 (from memory) - A whole CUDA program from a blank page   [L07 slides 22-26]
// REQUIRES: cuda
//
// PREDICT FIRST: list the five host-side steps of a CUDA program in order,
//                and write the kernel's first line (the global index) from memory.
//
// Without looking at ex5 or the README, write a complete program that:
//   - reads n from argv[1] (default 1000),
//   - fills int arrays a[i] = i and b[i] = 2 * i on the host,
//   - computes c[i] = a[i] + b[i] on the GPU (one thread per element,
//     256 threads per block, any n must work),
//   - checks every errors returned by the CUDA calls,
//   - copies c back and prints exactly these two lines:
//         c[last] = <value of c[n-1]>
//         mismatches: <number of i with c[i] != 3 * i>
//   - returns 0 if there were no mismatches.
// This is the exam's "outline a CUDA solution" question, done for real.
//
// Test:  make test M=12 T=ex8_cuda_vecadd_from_memory
// Run:   make run  M=12 EX=ex8_cuda_vecadd_from_memory ARGS=5000
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

__global__ void add(const int *a, const int *b, int *c, int n) {
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i < n) c[i] = a[i] + b[i];
}

int main(int argc, char **argv) {
    int n = argc > 1 ? atoi(argv[1]) : 1000;
    size_t bytes = (size_t)n * sizeof(int);
    int *a = (int *)malloc(bytes), *b = (int *)malloc(bytes), *c = (int *)malloc(bytes);
    for (int i = 0; i < n; i++) { a[i] = i; b[i] = 2 * i; }

    int *d_a, *d_b, *d_c;
    CUDA_CHECK(cudaMalloc((void **)&d_a, bytes));                        // 1. allocate
    CUDA_CHECK(cudaMalloc((void **)&d_b, bytes));
    CUDA_CHECK(cudaMalloc((void **)&d_c, bytes));
    CUDA_CHECK(cudaMemcpy(d_a, a, bytes, cudaMemcpyHostToDevice));       // 2. copy in
    CUDA_CHECK(cudaMemcpy(d_b, b, bytes, cudaMemcpyHostToDevice));

    int threads = 256, blocks = (n + threads - 1) / threads;
    add<<<blocks, threads>>>(d_a, d_b, d_c, n);                          // 3. launch
    CUDA_CHECK(cudaGetLastError());
    CUDA_CHECK(cudaDeviceSynchronize());

    CUDA_CHECK(cudaMemcpy(c, d_c, bytes, cudaMemcpyDeviceToHost));       // 4. copy out
    CUDA_CHECK(cudaFree(d_a));                                           // 5. free
    CUDA_CHECK(cudaFree(d_b));
    CUDA_CHECK(cudaFree(d_c));

    int wrong = 0;
    for (int i = 0; i < n; i++) wrong += c[i] != 3 * i;
    printf("c[last] = %d\n", c[n - 1]);
    printf("mismatches: %d\n", wrong);
    free(a); free(b); free(c);
    return wrong == 0 ? 0 : 1;
}
