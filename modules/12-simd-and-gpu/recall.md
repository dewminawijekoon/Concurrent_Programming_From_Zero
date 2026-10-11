# 12 · Recall sheet

Cover the right-hand column. Say each answer out loud, or write it on a
blank page. If you hesitate, re-read that part of the [README](README.md).

| Question | What you should be able to say or write |
|---|---|
| What is SIMD? [EXTRA] | Single Instruction, Multiple Data: one instruction does the same operation on several numbers held side by side in a vector register. |
| What is AVX, and how many numbers fit in one register? [EXTRA] | Intel/AMD's 256-bit SIMD instructions. One register holds 8 floats or 4 doubles. AVX-512 doubles that. |
| How does AVX raise parallelism? [EXTRA] | Each instruction does up to 8 float operations on one core, so work per clock tick goes up without extra threads. It only helps loops whose steps are independent. |
| Why does a SIMD loop need a remainder loop? [EXTRA] | The main loop handles groups of 8. When n is not a multiple of 8, the last n % 8 elements must be done one at a time. |
| `loadu` vs `load`? [EXTRA] | `_mm256_loadu_ps` works at any address. `_mm256_load_ps` needs an address that is a multiple of 32, otherwise it crashes. |
| Which loops does the compiler refuse to vectorize? [EXTRA] | Loops where step i needs step i-1; arrays that might overlap (fix with `restrict`); float sums unless you allow reordering with `-ffast-math`. |
| CPU vs GPU design? | CPU: few big clever cores, built for low latency on one task. GPU: thousands of simple cores, built for high throughput on many identical tasks. |
| Peak GFLOPS formula (L07 slide 8)? | cores × FLOPs per core per tick × GHz. CPU 4 × 4 × 2.5 = 40; GPU 448 × 1 × 1.15 = 515. |
| Why may the GPU still lose? | The data must be copied over PCI Express both ways, and dependent or branchy code cannot use thousands of cores. |
| Host vs device? | Host = CPU and its memory, runs normal C. Device = GPU and its separate memory, runs kernels with many threads. |
| What do `__global__`, `__device__`, `__host__` mean? | Kernel (runs on GPU, called from host, returns void); GPU helper called from GPU code; normal CPU function. |
| Global thread index? | `i = blockIdx.x * blockDim.x + threadIdx.x` |
| How many blocks for n elements and T threads per block? | `(n + T - 1) / T` (round up), and the kernel checks `if (i < n)`. |
| The five host steps? | cudaMalloc → cudaMemcpy host→device → kernel<<<blocks, threads>>> → cudaMemcpy device→host → cudaFree. |
| GPU memory kinds? | Registers (one thread, fastest), shared (one block, fast, `__shared__`), global (everyone + host, slow, big), constant (read-only, 64 KB, cached). |
| What does `__syncthreads()` do? | Waits until all threads of the **block** reach it. It does not wait for other blocks, and every thread of the block must reach it. |
| Why `atomicAdd`? | `*total += x` is read-add-write, so concurrent threads lose updates. `atomicAdd` does it as one indivisible step. |
| How do you check for CUDA errors? | Check the return value of every call; after a launch call `cudaGetLastError()` and `cudaDeviceSynchronize()`. |
| Matrix multiply on the GPU? | One thread per element of the result: col from x, row from y, loop over k, bounds check `row < N && col < N`. |
| What is a warp? [EXTRA] | 32 threads that execute the same instruction together. If they take different branches, both branches run one after the other (divergence). |
| What is coalescing? [EXTRA] | Threads of a warp reading neighbouring addresses are served by one wide memory transaction; scattered addresses need many. |
| Shared-memory reduction? [EXTRA] | Each thread stores its value in `part[t]`; in rounds with stride = half, quarter, …, threads t < stride add `part[t + stride]`; `__syncthreads()` after each round; thread 0 does one `atomicAdd`. |

**From memory:** write a complete CUDA vector-add program (exercise 8),
and the AVX loop for `c[i] = a[i] + b[i]` with its remainder loop.
