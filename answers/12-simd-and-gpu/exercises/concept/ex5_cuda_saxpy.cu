// Exercise 5 (core) - SAXPY on the GPU: the five steps of every CUDA program
//                                                      [L07 slides 22-26, 32-34, 47]
// REQUIRES: cuda
//
// PREDICT FIRST: n = 1,000,003 and 256 threads per block. How many blocks do
//                you launch? What goes wrong if the kernel has no "if (i < n)"?
//
// SAXPY means y = a*x + y for every element (Single-precision A times X Plus Y).
// Every CUDA program follows the same recipe (L07 slide 22):
//   1. allocate memory on the GPU          cudaMalloc
//   2. copy the inputs to the GPU          cudaMemcpy(..., cudaMemcpyHostToDevice)
//   3. launch the kernel                   kernel<<<blocks, threads>>>(...)
//   4. copy the results back               cudaMemcpy(..., cudaMemcpyDeviceToHost)
//   5. free the GPU memory                 cudaFree
// Fill in the kernel and the TODO steps in main(). The CPU version and the
// final check are given; do not change them.
//
// No NVIDIA GPU? See the module README, section 9 (Google Colab).
// Test:  make test M=12 T=ex5_cuda_saxpy
// Run:   make run  M=12 EX=ex5_cuda_saxpy          (or: nvcc -o saxpy ex5_cuda_saxpy.cu && ./saxpy)
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

// Wrap every CUDA call: if it fails, print where and why, then stop.
#define CUDA_CHECK(call)                                                        \
    do {                                                                        \
        cudaError_t err_ = (call);                                              \
        if (err_ != cudaSuccess) {                                              \
            fprintf(stderr, "CUDA error at %s:%d: %s\n", __FILE__, __LINE__,    \
                    cudaGetErrorString(err_));                                  \
            exit(1);                                                            \
        }                                                                       \
    } while (0)

// Runs on the GPU, once per thread.
__global__ void saxpy_kernel(int n, float a, const float *x, float *y) {
    int i = blockIdx.x * blockDim.x + threadIdx.x;   // my element
    if (i < n)                                       // the last block may stick out past n
        y[i] = a * x[i] + y[i];
}

// ---------------------------------------------------------------- given
static void saxpy_cpu(int n, float a, const float *x, float *y) {
    for (int i = 0; i < n; i++) y[i] = a * x[i] + y[i];
}

int main(int argc, char **argv) {
    int n = argc > 1 ? atoi(argv[1]) : 1000003;   // deliberately NOT a multiple of 256
    float a = 2.5f;
    size_t bytes = (size_t)n * sizeof(float);
    float *x = (float *)malloc(bytes), *y = (float *)malloc(bytes), *y_ref = (float *)malloc(bytes);
    for (int i = 0; i < n; i++) {
        x[i] = (float)(i % 100) * 0.01f;
        y[i] = (float)(i % 7);
        y_ref[i] = y[i];
    }
    saxpy_cpu(n, a, x, y_ref);                     // the answer we expect

    float *d_x = NULL, *d_y = NULL;                // "d_" = lives in device (GPU) memory
    // ------------------------------------------------------------ your code
    // Step 1: allocate d_x and d_y on the GPU.
    CUDA_CHECK(cudaMalloc((void **)&d_x, bytes));
    CUDA_CHECK(cudaMalloc((void **)&d_y, bytes));

    // Step 2: copy x and y to the GPU.
    CUDA_CHECK(cudaMemcpy(d_x, x, bytes, cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemcpy(d_y, y, bytes, cudaMemcpyHostToDevice));

    // Step 3: launch enough blocks of 256 threads to cover all n elements.
    int threads = 256;
    int blocks = (n + threads - 1) / threads;      // round up
    saxpy_kernel<<<blocks, threads>>>(n, a, d_x, d_y);

    // ------------------------------------------------------------ given
    CUDA_CHECK(cudaGetLastError());                // did the launch itself fail?
    CUDA_CHECK(cudaDeviceSynchronize());           // wait for the kernel; reports errors inside it

    // ------------------------------------------------------------ your code
    // Step 4: copy d_y back into y.
    CUDA_CHECK(cudaMemcpy(y, d_y, bytes, cudaMemcpyDeviceToHost));

    // Step 5: free the GPU memory.
    CUDA_CHECK(cudaFree(d_x));
    CUDA_CHECK(cudaFree(d_y));

    // ------------------------------------------------------------ given: check
    int wrong = 0;
    for (int i = 0; i < n; i++)
        if (fabsf(y[i] - y_ref[i]) > 1e-5f * (1.0f + fabsf(y_ref[i]))) wrong++;
    printf("n = %d, blocks = %d, threads per block = %d\n", n, blocks, threads);
    printf("y[0] = %.4f, y[n-1] = %.4f (expected %.4f, %.4f)\n", y[0], y[n - 1], y_ref[0], y_ref[n - 1]);
    printf("mismatches: %d of %d\n", wrong, n);
    free(x); free(y); free(y_ref);
    return wrong == 0 ? 0 : 1;
}
