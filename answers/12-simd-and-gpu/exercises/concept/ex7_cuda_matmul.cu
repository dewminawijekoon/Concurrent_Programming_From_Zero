// Exercise 7 (core) - Matrix multiplication on the GPU            [L07 slides 27-34]
// REQUIRES: cuda
//
// PREDICT FIRST: for N x N matrices, how many threads does this kernel use,
//                and how many multiply-adds does each thread do? Why can all
//                threads run at the same time without any lock?
//
// a = b * c for square N x N matrices stored in row-major order (L07 slide 28):
// element (row, col) of a matrix m lives at m[row * N + col].
// Each GPU thread computes ONE element of a:
//     a[row][col] = sum over k of b[row][k] * c[k][col]
//
// L07 uses a 2-D grid of 1-D blocks and assumes N is a multiple of 32. We use
// 2-D blocks of 16 x 16 threads and allow ANY N, so the kernel needs a bounds
// check: the blocks on the right and bottom edges stick out past the matrix.
//
// Test:  make test M=12 T=ex7_cuda_matmul
// Run:   make run  M=12 EX=ex7_cuda_matmul ARGS=500
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define CUDA_CHECK(call)                                                        \
    do {                                                                        \
        cudaError_t err_ = (call);                                              \
        if (err_ != cudaSuccess) {                                              \
            fprintf(stderr, "CUDA error at %s:%d: %s\n", __FILE__, __LINE__,    \
                    cudaGetErrorString(err_));                                  \
            exit(1);                                                            \
        }                                                                       \
    } while (0)

#define TILE 16   // a block is TILE x TILE threads

__global__ void matmul_kernel(float *a, const float *b, const float *c, int N) {
    int col = blockIdx.x * blockDim.x + threadIdx.x;   // x direction = columns
    int row = blockIdx.y * blockDim.y + threadIdx.y;   // y direction = rows
    if (row < N && col < N) {                          // edge blocks stick out
        float sum = 0.0f;                              // keep the running sum in a register
        for (int k = 0; k < N; k++)
            sum += b[row * N + k] * c[k * N + col];
        a[row * N + col] = sum;                        // one write to global memory at the end
    }
}

// ---------------------------------------------------------------- given
static void matmul_cpu(float *a, const float *b, const float *c, int N) {
    for (int i = 0; i < N; i++)
        for (int j = 0; j < N; j++) {
            float sum = 0.0f;
            for (int k = 0; k < N; k++) sum += b[i * N + k] * c[k * N + j];
            a[i * N + j] = sum;
        }
}

int main(int argc, char **argv) {
    int N = argc > 1 ? atoi(argv[1]) : 300;   // 300 is not a multiple of 16
    size_t bytes = (size_t)N * N * sizeof(float);
    float *a = (float *)malloc(bytes), *b = (float *)malloc(bytes);
    float *c = (float *)malloc(bytes), *a_ref = (float *)malloc(bytes);
    for (int i = 0; i < N * N; i++) {
        b[i] = (float)((i * 7) % 11) * 0.1f;
        c[i] = (float)((i * 3) % 13) * 0.1f - 0.5f;
        a[i] = -999.0f;                        // garbage, so a missing write shows up
    }
    matmul_cpu(a_ref, b, c, N);

    float *d_a = NULL, *d_b = NULL, *d_c = NULL;
    CUDA_CHECK(cudaMalloc((void **)&d_a, bytes));
    CUDA_CHECK(cudaMalloc((void **)&d_b, bytes));
    CUDA_CHECK(cudaMalloc((void **)&d_c, bytes));
    CUDA_CHECK(cudaMemcpy(d_b, b, bytes, cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemcpy(d_c, c, bytes, cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemcpy(d_a, a, bytes, cudaMemcpyHostToDevice));

    // ------------------------------------------------------------ your code
    // Blocks of TILE x TILE threads, and enough blocks in x and y to cover N.
    dim3 block(TILE, TILE);
    dim3 grid((N + TILE - 1) / TILE, (N + TILE - 1) / TILE);
    matmul_kernel<<<grid, block>>>(d_a, d_b, d_c, N);

    // ------------------------------------------------------------ given
    CUDA_CHECK(cudaGetLastError());
    CUDA_CHECK(cudaDeviceSynchronize());
    CUDA_CHECK(cudaMemcpy(a, d_a, bytes, cudaMemcpyDeviceToHost));
    CUDA_CHECK(cudaFree(d_a));
    CUDA_CHECK(cudaFree(d_b));
    CUDA_CHECK(cudaFree(d_c));

    int wrong = 0;
    for (int i = 0; i < N * N; i++)
        if (fabsf(a[i] - a_ref[i]) > 1e-3f * (1.0f + fabsf(a_ref[i]))) wrong++;
    printf("N = %d, grid = %u x %u blocks of %u x %u threads\n", N, grid.x, grid.y, block.x, block.y);
    printf("a[0][0] = %.4f, a[N-1][N-1] = %.4f (expected %.4f, %.4f)\n",
           a[0], a[N * N - 1], a_ref[0], a_ref[N * N - 1]);
    printf("mismatches: %d of %d\n", wrong, N * N);
    free(a); free(b); free(c); free(a_ref);
    return wrong == 0 ? 0 : 1;
}
