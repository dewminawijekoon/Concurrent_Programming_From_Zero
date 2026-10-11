# 12 · Quiz answers: SIMD and GPUs

These answers are about the length a marker expects for the marks shown.
"Why" is always part of the answer.

## A. True / false

1. **False.** A 256-bit register holds 256 / 64 = **4** doubles, or 8
   floats. So one AVX instruction adds 4 pairs of doubles (8 pairs of
   floats).
2. **False.** Each GPU core is *simpler and slower* than a CPU core (lower
   clock, fewer FLOPs per tick, no clever tricks like guessing branches).
   The GPU wins on *throughput*: it has hundreds or thousands of cores
   working at once (L07 slides 7–8: 448 × 1 × 1.15 = 515 GFLOPS against
   4 × 4 × 2.5 = 40).
3. **False.** A `__global__` function must return `void` (L07 slide 35). It
   writes its results into device memory, and the host copies them back with
   `cudaMemcpy(..., cudaMemcpyDeviceToHost)`.
4. **False.** It is a barrier for the threads of **one block** only
   (slide 41). Threads in different blocks cannot wait for each other inside
   a kernel. To wait for the whole grid, end the kernel and launch another.
5. **False.** A kernel launch is **asynchronous** (slide 35): the host goes
   on immediately. Call `cudaDeviceSynchronize()` to wait. (A following
   `cudaMemcpy` from the device also waits for the kernel.)
6. **False.** `d_x` holds an address in the **GPU's** memory, which is a
   separate memory space (slide 22). The host must copy the data back with
   `cudaMemcpy` first. Reading `d_x[0]` on the host crashes or gives garbage.
7. **False.** It only vectorizes when the result is guaranteed to stay the
   same. A loop like `x[i] += x[i-1]` (each step needs the previous one)
   cannot be split into lanes, and a float sum needs permission to change
   the order of additions (`-ffast-math`). Possible overlap between arrays
   can also stop it (fixed with `restrict`).
8. **False.** Data must be copied over the PCI Express bus first, and the
   results back (slide 14). If the kernel does little work per byte copied
   (like adding two vectors), the copy costs more than the GPU saves.
   Dependent or branchy code also never reaches the GPU's peak.

## B. Calculations and traces

9. Floating-point addition **rounds** after every step, so the order
   matters: (a + b) + c can differ from a + (b + c) in the last digits.
   Adding 8 at a time means keeping 8 separate partial sums and combining
   them at the end, which is a different order from the loop's
   left-to-right order. C says the program must behave as written, so gcc
   may not change the order on its own. It can still *load* 8 values at
   once (that is why it says "vectorized"), but then it adds them one by
   one in the original order (`vaddss`). `-ffast-math` tells gcc "I accept
   small rounding differences", and then it uses 8 partial sums (`vaddps`).
10. GPU: 2560 × 2 × 1.5 = **7680 GFLOPS**. CPU: 8 × 32 × 3.0 = **768
    GFLOPS**. Ratio **10**. The CPU's 32: each 256-bit unit handles 8
    floats at once (SIMD), a fused multiply-add counts as 2 FLOPs, and
    there are 2 units: 2 × 8 × 2 = 32.
11. Data copied: 2 inputs + 1 result = 3 × 4096 × 4096 × 4 bytes =
    201,326,592 bytes ≈ 0.2013 GB. Copy time = 0.2013 / 12 ≈ **0.0168 s**.
    Total GPU time = 0.05 + 0.0168 = 0.0668 s. Real speedup = 3.0 / 0.0668
    ≈ **44.9**. With free copies: 3.0 / 0.05 = **60**. Here the copies cost
    a quarter of the time. For a cheap calculation like vector addition the
    copies would dominate completely.
12. Blocks = ⌈5,000,000 / 256⌉ = ⌈19531.25⌉ = **19532**. Threads started =
    19532 × 256 = 5,000,192, so **192** threads have i ≥ n and do nothing
    (they fail the `if (i < n)` check). With `n / 256` = 19531 blocks
    (rounded down), only 4,999,936 threads start, so the **last 64
    elements are never processed**, and nothing reports an error.
13. (a) 2 × 8 + 5 = **21**. (b) 30 / 8 = 3 remainder 6, so **block 3,
    thread 6**. (c) `gridDim.x = 4` (blocks), `blockDim.x = 8` (threads
    per block).
14. Start `2 7 1 8 2 8 1 8`.
    Stride 4: `part[0..3] = 2+2, 7+8, 1+1, 8+8 = 4 15 2 16`.
    Stride 2: `part[0..1] = 4+2, 15+16 = 6 31`.
    Stride 1: `part[0] = 6+31 = 37`.
    Check: 2+7+1+8+2+8+1+8 = 37. Three rounds, because log₂ 8 = 3.
15. `*total += v` is three steps: read, add, write.
    - **8**: A reads 0, A writes 3, B reads 3, B writes 8 (or B first, then A).
    - **3**: A reads 0, B reads 0, B writes 5, A writes 3 (B's update is lost).
    - **5**: A reads 0, B reads 0, A writes 3, B writes 5 (A's update is lost).
    With `atomicAdd` each read-add-write is one indivisible step, so the
    other thread cannot slip in between: the only possible result is **8**.
    This is the same lost-update race as `counter++` in module 01.

## C. Explain and design

16. *(6 marks: what it is 2, how it raises parallelism with an example 2,
    limits 2.)*
    AVX (Advanced Vector Extensions) is a set of **SIMD** instructions on
    Intel/AMD CPUs: Single Instruction, Multiple Data. It adds 256-bit
    **vector registers**, each holding several numbers side by side: 8
    floats or 4 doubles. One AVX instruction applies the same operation to
    all of them at once. For example, `c[i] = a[i] + b[i]` for i = 0..7 is
    done by one load of `a[0..7]`, one load of `b[0..7]`, one `vaddps` and
    one store, instead of 8 separate additions.
    This increases parallelism *inside a single core*: there are no extra
    threads, but each instruction does up to 8 times more work, so the
    peak FLOPs per clock tick rises by up to 8 (it is part of the "FLOPs
    per core per tick" in the L07 GFLOPS formula). It combines with
    multithreading: 4 cores × 8 lanes = 32 additions per tick.
    It only helps for **data-parallel** loops: the same operation on
    neighbouring, independent elements. A loop where step i needs step
    i-1's result (`x[i] += x[i-1]`) cannot use it. Branchy code and
    memory-bound code gain little. The programmer gets it either from the
    compiler (`-O3 -march=native`) or by writing intrinsics such as
    `_mm256_add_ps`, with a remainder loop when n is not a multiple of 8.
17. Matrix multiplication has N² **independent** output elements, each
    doing the same simple multiply-add loop with regular memory access.
    That fits the GPU's design: thousands of simple cores doing the same
    thing, high throughput, no locks needed. A linked list is a **chain**:
    you can only find node k+1 after reading node k, so there is nothing
    to spread over thousands of cores, and each step waits for slow memory.
    Code full of `if`s makes the threads of a warp take different paths, and
    the GPU then runs the paths one after the other (divergence). A CPU
    core, with its branch prediction and big caches, is built for exactly
    that kind of code.
18. The 32 threads of a warp execute the same instruction together. Here
    even and odd threads sit in the **same warp** but take different
    branches, so the warp first runs `heavy` (odd threads idle), then runs
    `light` (even threads idle). Each warp pays for both paths: that is
    **divergence**. Fix: make the decision the same for a whole warp,
    for example give the even `i` to the first half of the threads and the
    odd `i` to the second half (`j = (i < n/2) ? 2*i : 2*(i - n/2) + 1`),
    so whole warps take the same branch. Or sort the work so similar items
    are next to each other.
19. **Kernel A.** Neighbouring threads of a warp read neighbouring
    addresses, so the GPU fetches the 32 values in one or two wide memory
    transactions (**coalesced** access). In kernel B the 32 addresses are
    32 floats (128 bytes) apart, so each value needs its own transaction:
    up to 32 times more memory traffic for the same useful data.

## D. Code and outlines

20. *(10 marks: decomposition 2, kernel 3, combining safely 2, host code 2,
    remark 1.)*
    **Decomposition.** Each number k in 1..N is independent: its digit sum
    depends only on k. Use a fixed grid (for example 1024 blocks × 256
    threads) and a **grid-stride loop**, so each thread handles about
    N / 262144 numbers. Use 64-bit integers: N = 10⁹ needs `long long`
    indices, and the total (about 4 × 10¹⁰) does not fit in 32 bits.

    ```c
    #define T 256
    __device__ int digit_sum(long long k) {           // helper, runs on the GPU
        int s = 0;
        while (k > 0) { s += k % 10; k /= 10; }
        return s;
    }

    __global__ void digits_kernel(long long N, unsigned long long *total) {
        __shared__ unsigned long long part[T];         // one slot per thread of the block
        long long first = (long long)blockIdx.x * blockDim.x + threadIdx.x + 1;
        long long step  = (long long)gridDim.x * blockDim.x;     // total number of threads
        unsigned long long mine = 0;                   // private sum in a register: no race
        for (long long k = first; k <= N; k += step) mine += digit_sum(k);

        part[threadIdx.x] = mine;                      // combine inside the block
        __syncthreads();
        for (int s = blockDim.x / 2; s > 0; s /= 2) {  // tree reduction in shared memory
            if (threadIdx.x < s) part[threadIdx.x] += part[threadIdx.x + s];
            __syncthreads();
        }
        if (threadIdx.x == 0) atomicAdd(total, part[0]);   // one atomic per block
    }

    // host
    unsigned long long *d_total, total;
    cudaMalloc((void **)&d_total, sizeof total);
    cudaMemset(d_total, 0, sizeof total);              // the sum starts at 0
    digits_kernel<<<1024, T>>>(N, d_total);
    check cudaGetLastError() and cudaDeviceSynchronize()
    cudaMemcpy(&total, d_total, sizeof total, cudaMemcpyDeviceToHost);
    cudaFree(d_total);
    print total
    ```

    **Combining safely.** Many threads adding to one variable with `+=` is
    a data race (lost updates). Each thread first sums into its own
    register. The block then adds its 256 partial sums in shared memory
    with `__syncthreads()` between rounds, and thread 0 does a single
    `atomicAdd`. That gives 1024 atomic operations instead of 10⁹.
    **Remark.** Nothing needs to be copied to the GPU except N, so the copy
    cost is tiny. The work per number is almost the same everywhere
    (9 or 10 digits), so a simple cyclic (grid-stride) split balances
    well. (This program was checked on our CPU emulator: N = 12 gives 51,
    N = 1000 gives 13501.)
21. *(Kernel 4, launch 2.)*
    ```c
    __global__ void matmul(const float *A, const float *B, float *C, int N) {
        int col = blockIdx.x * blockDim.x + threadIdx.x;
        int row = blockIdx.y * blockDim.y + threadIdx.y;
        if (row < N && col < N) {                      // edge blocks stick out
            float sum = 0.0f;
            for (int k = 0; k < N; k++) sum += A[row * N + k] * B[k * N + col];
            C[row * N + col] = sum;
        }
    }
    dim3 block(16, 16);                                // 256 threads
    dim3 grid((N + 15) / 16, (N + 15) / 16);           // round up in both directions
    matmul<<<grid, block>>>(d_A, d_B, d_C, N);
    ```
    One thread per element of C; no locks are needed because each thread
    writes a different element and A, B are only read.
22. ```c
    #include <immintrin.h>
    void mul_add(const float *a, const float *b, float *c, size_t n) {
        size_t i = 0;
        for (; i + 8 <= n; i += 8) {
            __m256 va = _mm256_loadu_ps(a + i);        // loadu: any alignment
            __m256 vb = _mm256_loadu_ps(b + i);
            __m256 vc = _mm256_loadu_ps(c + i);
            _mm256_storeu_ps(c + i, _mm256_add_ps(_mm256_mul_ps(va, vb), vc));
        }
        for (; i < n; i++) c[i] = a[i] * b[i] + c[i];  // remainder: the last n % 8
    }
    ```
    (With `-mfma`, `_mm256_fmadd_ps(va, vb, vc)` does the multiply and add
    in one instruction.)
