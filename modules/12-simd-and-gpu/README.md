# 12 · SIMD and GPUs (CUDA)

| | |
|---|---|
| **Time** | about 5 hours (plus Colab setup if you have no NVIDIA GPU) |
| **You should know first** | module 01 (what a race is), module 02 (speedup), module 03 (threads in C) |
| **Lectures** | L07 *Introduction to Accelerators* (slides 1–47; slides 48–55 on Xeon Phi are background only) |
| **Helps with** | exam questions "explain AVX and how it increases parallelism" (6 marks, [EXTRA] here), "outline a CUDA solution" (10 marks), GFLOPS calculations; module 16 |

**Goal:** understand the two big ways hardware does many calculations at
once, *wide instructions* (SIMD/AVX) and *many simple cores* (GPUs), and
write and outline real CUDA programs.

Words in **bold** link to the [glossary](../../GLOSSARY.md) the first time
they appear. There is no lab for this module; the exercises stand on their
own.

---

## 1. The problem

You want to add two arrays of one million numbers. A plain loop on one CPU
core does one addition per step:

```c
for (int i = 0; i < n; i++) c[i] = a[i] + b[i];   // one add at a time
```

Every step is independent: `c[5]` does not need `c[4]`. So, in principle,
all one million additions could happen at the same moment. The hardware
offers two ways to do many of them at once:

1. **Wider instructions.** One instruction adds 8 pairs of numbers instead
   of 1. This is **SIMD** (section 2.1).
2. **Many more cores.** A GPU has thousands of small cores. Each one adds
   one pair, and they all work at the same time (section 2.2).

Both only help when the work is made of many **independent** pieces of the
same kind. That is called **data parallelism**: the same operation applied
to lots of different data. L07 slide 14 says this directly: SIMD-style data
parallelism is the key to getting speed from a GPU.

## 2. The idea

### 2.1 [EXTRA] SIMD: one instruction, many data

**SIMD** stands for *Single Instruction, Multiple Data*. One instruction
works on a small group of numbers at once.

To do that, the CPU has **vector registers**. A normal register holds one
number. A vector register is wider and holds several numbers side by side.
Each position is called a **lane**.

```
a 256-bit AVX register holding 8 floats (each float is 32 bits):

   lane:   0     1     2     3     4     5     6     7
        +-----+-----+-----+-----+-----+-----+-----+-----+
  va =  | 1.0 | 2.0 | 3.0 | 4.0 | 5.0 | 6.0 | 7.0 | 8.0 |
        +-----+-----+-----+-----+-----+-----+-----+-----+
  vb =  | 0.5 | 0.5 | 0.5 | 0.5 | 0.5 | 0.5 | 0.5 | 0.5 |
        +-----+-----+-----+-----+-----+-----+-----+-----+
  ONE instruction (vaddps): every lane adds at the same time
        +-----+-----+-----+-----+-----+-----+-----+-----+
  vc =  | 1.5 | 2.5 | 3.5 | 4.5 | 5.5 | 6.5 | 7.5 | 8.5 |
        +-----+-----+-----+-----+-----+-----+-----+-----+
```

Intel and AMD CPUs have added wider and wider SIMD instructions over the years:

| Name | Register width | floats per instruction | doubles per instruction |
|---|---|---|---|
| SSE | 128 bits | 4 | 2 |
| **AVX**, AVX2 | 256 bits | 8 | 4 |
| AVX-512 | 512 bits | 16 | 8 |

**AVX** means *Advanced Vector Extensions*. AVX introduced the 256-bit
registers for floating-point numbers. AVX2 extended them to whole numbers
(integers) and added a few more instructions. L07 slide 52 mentions
AVX-512 on the Xeon Phi (Knights Landing).

**How AVX increases parallelism.** It is still one core running one stream
of instructions. But each instruction now does 8 float operations instead
of 1. The parallelism is *inside* the instruction. So a core can finish up
to 8 times more work per clock tick, without adding any threads. You can
combine it with threads: 4 cores × 8 lanes = 32 additions per tick.

**The catch.** SIMD only works when the same operation is applied to
neighbouring, independent elements. A loop where step `i` needs the result
of step `i-1` (like `x[i] += x[i-1]`) cannot be split into lanes, because
lane 1 would need lane 0's answer from the same instruction.

### 2.2 GPU vs CPU: two different designs

A CPU and a GPU spend their chips on different things (L07 slide 7).

| | CPU | GPU |
|---|---|---|
| Number of cores | a few (4–64) | thousands |
| Each core | big and clever: guesses branches, runs instructions out of order, large caches | small and simple |
| Designed for | **low latency**: finish *one* task as fast as possible | **high throughput**: finish as *many* tasks per second as possible |
| Good at | branchy, irregular code; one long chain of steps | the same simple operation on huge amounts of data |

**Latency** is how long one job takes. **Throughput** is how many jobs
finish per second. A sports car has low latency (one person, fast). A bus
has high throughput (fifty people, slower). A GPU is the bus.

L07 slides 5–6 tell the history: GPUs were built to draw graphics, where
every pixel needs the same calculation. People then started using them for
general calculations. This is called **GPGPU**: *General-Purpose computing
on GPUs*.

### 2.3 Peak speed in GFLOPS

A **FLOP** is one floating-point operation, like one multiplication of two
decimals. **FLOPS** means FLOPs per second, and **GFLOPS** means billions
(10⁹) of them per second. L07 slide 8 computes the *peak* (best possible)
speed like this:

```
peak GFLOPS = number of cores x FLOPs per core per clock tick x clock speed in GHz
```

| | CPU | GPU |
|---|---|---|
| cores | 4 | 448 |
| FLOPs per core per tick | 4 | 1 |
| clock (GHz) | 2.5 | 1.15 |
| **peak** | 4 × 4 × 2.5 = **40 GFLOPS** | 448 × 1 × 1.15 = **515.2 GFLOPS** |

Each GPU core is weaker (1 FLOP per tick instead of 4, and a slower clock),
but there are 112 times more of them. So the GPU's peak is about 13 times
higher. (515 GFLOPS is the double-precision peak of the Tesla 2070 on
slide 4.)

Where does a CPU's "FLOPs per tick" come from? From SIMD. For example, a
128-bit SSE unit that does one addition and one multiplication of 2
doubles per tick gives 2 × 2 = 4. A modern core with two 256-bit units,
each doing a *fused multiply-add* (a multiply and an add in one
instruction, 2 FLOPs) on 8 floats, gives 2 × 8 × 2 = 32 float FLOPs per
tick.

**Peak is not what you get.** Real programs also wait for memory. And a
GPU program must first copy its data to the GPU over the PCI Express bus.
L07 slide 14 warns that this copying can cost more than it saves. Exercise
1 makes you count it.

## 3. The mechanism

### 3.1 [EXTRA] AVX in C: let the compiler do it, or do it by hand

**Option 1: auto-vectorization.** Write a normal loop and let the compiler
turn it into SIMD instructions.

```bash
gcc -O3 -march=native -fopt-info-vec-optimized prog.c
# prog.c:12:23: optimized: loop vectorized using 32 byte vectors
```

`-O3` turns on the vectorizer fully. `-march=native` lets gcc use every
instruction your CPU has (AVX2 here). `-fopt-info-vec-optimized` prints a
note for every loop it vectorized. 32-byte vectors are 256-bit AVX
registers.

The compiler only vectorizes a loop when it can prove the result stays
exactly the same. Three things often stop it:

| Problem | Example | Why it blocks SIMD |
|---|---|---|
| step i needs step i-1 | `x[i] += x[i-1];` | the lanes would need each other's results in the same instruction |
| arrays might overlap | `void f(float *y, float *x)` with `y = x + 1` | writing `y` could change `x`; add `restrict` to promise they don't overlap |
| float order would change | `s += x[i];` | SIMD adds in a different order, and float addition rounds differently in a different order; gcc needs `-ffast-math` to allow that |

The last one surprises people: (a + b) + c and a + (b + c) can give
slightly different floats, because each addition rounds. Exercise 3 lets
you watch all three.

**Option 2: intrinsics.** *Intrinsics* are special C functions that each
become one SIMD instruction. They come from `<immintrin.h>`. The type
`__m256` is "a 256-bit register holding 8 floats".

```c
#include <immintrin.h>
// c[i] = a[i] + b[i], 8 at a time; compile with -mavx2
void add(const float *a, const float *b, float *c, size_t n) {
    size_t i = 0;
    for (; i + 8 <= n; i += 8) {                       // main loop: whole groups of 8
        __m256 va = _mm256_loadu_ps(a + i);            // load a[i..i+7]
        __m256 vb = _mm256_loadu_ps(b + i);            // load b[i..i+7]
        _mm256_storeu_ps(c + i, _mm256_add_ps(va, vb)); // 8 adds, store c[i..i+7]
    }
    for (; i < n; i++) c[i] = a[i] + b[i];             // remainder loop: the last n % 8
}
```

Three details matter:

- **The remainder loop.** If n = 1003, the main loop handles 1000 elements
  (125 groups of 8). The last 3 need a normal loop. Forget it and the last
  elements are never computed. Run the main loop too far and you read and
  write past the end of the arrays.
- **Alignment.** Memory addresses that are a multiple of 32 are
  *32-byte aligned*. `_mm256_load_ps` (no `u`) requires that, and crashes
  otherwise. `_mm256_loadu_ps` (`u` = unaligned) works for any address. On
  modern CPUs it is just as fast when the address happens to be aligned.
  Use `loadu`/`storeu` unless you allocated the memory aligned yourself
  (for example with `aligned_alloc(32, bytes)`).
- **Reductions** like a dot product keep 8 separate running sums, one per
  lane, and add the 8 together once at the end (a *horizontal sum*). The
  order of additions changes, so the answer can differ from the plain loop
  in the last digits.

### 3.2 CUDA: host and device

**CUDA** is NVIDIA's way to program its GPUs: C with a few additions,
compiled by `nvcc` (L07 slides 13–15). A CUDA program has two parts
(slide 16):

- the **host**: the CPU and its memory. It runs normal C code, one thread.
- the **device**: the GPU and its own memory. It runs many threads.

A function that runs on the GPU is called a **kernel**. You mark it with
`__global__` and call it from the host with an extra `<<< >>>` part that
says how many threads to start (slide 18):

```c
__global__ void hello(void) { }        // runs on the GPU
int main(void) {
    hello<<<1, 1>>>();                  // start 1 block of 1 thread
    cudaDeviceSynchronize();            // wait for the GPU to finish
}
```

L07 slide 35 lists three function markers ("qualifiers"):

| Marker | Runs on | Called from | Notes |
|---|---|---|---|
| `__global__` | device | host | a kernel; must return `void`; the call does not wait (it is *asynchronous*) |
| `__device__` | device | device | a helper for kernels |
| `__host__` | host | host | a normal C function (the default) |

`__host__ __device__` together compiles a function for both sides.
`__global__ __host__` together is not allowed.

### 3.3 Grids, blocks and threads

When you launch a kernel, CUDA starts a **grid** of threads. The grid is
split into **blocks**, and each block has the same number of **threads**
(L07 slides 17–21):

```
kernel<<<3, 4>>>(...)    3 blocks, 4 threads per block = 12 threads

           block 0          block 1          block 2
        +--+--+--+--+    +--+--+--+--+    +--+--+--+--+
thread: |0 |1 |2 |3 |    |0 |1 |2 |3 |    |0 |1 |2 |3 |     threadIdx.x
        +--+--+--+--+    +--+--+--+--+    +--+--+--+--+
global: |0 |1 |2 |3 |    |4 |5 |6 |7 |    |8 |9 |10|11|     blockIdx.x * blockDim.x + threadIdx.x
```

Every thread runs the **same kernel code**. It finds out *which* thread it
is from built-in variables (slide 38):

| Variable | Meaning | in the picture |
|---|---|---|
| `threadIdx.x` | my position inside my block | 0–3 |
| `blockIdx.x` | my block's position in the grid | 0–2 |
| `blockDim.x` | threads per block | 4 |
| `gridDim.x` | blocks in the grid | 3 |

The most important line in CUDA turns these into **one number per
thread**, used to pick the element that thread handles:

```c
int i = blockIdx.x * blockDim.x + threadIdx.x;   // "blocks before me" x "threads per block" + "my place in my block"
```

Block 2, thread 1 gets i = 2 × 4 + 1 = 9.

**Rounding up, and the bounds check.** For n = 10 elements and 4 threads
per block, you need 10 / 4 = 2.5 blocks. You can't start half a block, so
you round **up** to 3 blocks: `blocks = (n + threads - 1) / threads`. That
starts 12 threads for 10 elements. Threads 10 and 11 must do nothing, so
every kernel checks:

```c
if (i < n) c[i] = a[i] + b[i];
```

Without the check, threads 10 and 11 write past the end of the array.

Blocks and grids can also be 2-D or 3-D, using the type `dim3`
(slides 20, 37). Then `.y` (and `.z`) are used too, which is natural for
matrices (section 3.6). A block can have at most 1024 threads on current
GPUs.

**Grid-stride loop** [EXTRA]. If you want a fixed number of threads to
handle any n, each thread loops:

```c
for (int i = blockIdx.x * blockDim.x + threadIdx.x; i < n; i += gridDim.x * blockDim.x)
    c[i] = a[i] + b[i];       // jump ahead by the total number of threads each time
```

Exercise 4 checks all of this arithmetic in plain C, no GPU needed.

### 3.4 Memory: the GPU has its own

The host and the device have **separate memory** (slide 22). The GPU
cannot read your `malloc`'ed array, and the CPU cannot read GPU memory
directly. So every CUDA program follows five steps:

| Step | Call | What it does |
|---|---|---|
| 1 | `cudaMalloc((void **)&d_x, bytes)` | reserve memory on the GPU; `d_x` now holds a GPU address |
| 2 | `cudaMemcpy(d_x, x, bytes, cudaMemcpyHostToDevice)` | copy inputs host → GPU |
| 3 | `kernel<<<blocks, threads>>>(d_x, ...)` | run the kernel on the GPU copies |
| 4 | `cudaMemcpy(x, d_x, bytes, cudaMemcpyDeviceToHost)` | copy results GPU → host |
| 5 | `cudaFree(d_x)` | give the GPU memory back |

Watch the argument order of `cudaMemcpy`: **destination first**, like
`memcpy`. A common naming habit is `d_` for device pointers and `h_` (or
nothing) for host pointers, so you never mix them up. A device pointer
used on the host (`d_x[0]`) crashes or reads garbage.

Here is the whole thing for vector addition (our own version of the idea on
slides 25–26, with error checks):

```c
__global__ void add(const int *a, const int *b, int *c, int n) {
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i < n) c[i] = a[i] + b[i];
}

int main(void) {
    int n = 1000;  size_t bytes = n * sizeof(int);
    int *a = malloc(bytes), *b = malloc(bytes), *c = malloc(bytes);
    for (int i = 0; i < n; i++) { a[i] = i; b[i] = 2 * i; }

    int *d_a, *d_b, *d_c;
    cudaMalloc((void **)&d_a, bytes);  cudaMalloc((void **)&d_b, bytes);  cudaMalloc((void **)&d_c, bytes);
    cudaMemcpy(d_a, a, bytes, cudaMemcpyHostToDevice);
    cudaMemcpy(d_b, b, bytes, cudaMemcpyHostToDevice);

    int threads = 256, blocks = (n + threads - 1) / threads;   // 4 blocks
    add<<<blocks, threads>>>(d_a, d_b, d_c, n);

    cudaMemcpy(c, d_c, bytes, cudaMemcpyDeviceToHost);         // waits for the kernel first
    cudaFree(d_a);  cudaFree(d_b);  cudaFree(d_c);
}
```

Inside the GPU there are several kinds of memory (slides 23, 36). Faster
memory is smaller and seen by fewer threads:

| Memory | Who can see it | Speed | Size | How you get it |
|---|---|---|---|---|
| **registers** | one thread | fastest | a few hundred bytes per thread | ordinary local variables in a kernel |
| **shared memory** | all threads of **one block** | very fast (on the chip) | tens of KB per block | `__shared__ float s[256];` inside a kernel |
| **global memory** | every thread, and the host via `cudaMemcpy` | slow (hundreds of clock ticks) | gigabytes | `cudaMalloc`, or `__device__` variables |
| **constant memory** | every thread, read-only | fast when all threads read the same value (it is cached) | 64 KB | `__constant__` variables |

Shared memory lives as long as its block; global and constant memory live
for the whole program (slide 36).

### 3.5 Errors and waiting

Every CUDA function returns an error code of type `cudaError_t`. A kernel
launch returns nothing, so you ask afterwards with `cudaGetLastError()`
(slide 47). A wrong launch (for example 0 blocks, or more than 1024 threads
per block) is only reported this way. If you never check, the kernel
simply doesn't run and you get garbage without any message. That is the
number one CUDA beginner bug. Our exercises wrap every call:

```c
#define CUDA_CHECK(call) do { cudaError_t e = (call); if (e != cudaSuccess) {   \
    fprintf(stderr, "CUDA error at %s:%d: %s\n", __FILE__, __LINE__,             \
            cudaGetErrorString(e)); exit(1); } } while (0)

kernel<<<blocks, threads>>>(...);
CUDA_CHECK(cudaGetLastError());        // was the launch OK?
CUDA_CHECK(cudaDeviceSynchronize());   // wait for the kernel; reports errors that happened inside it
```

A kernel launch is **asynchronous**: the host goes straight on while the GPU
works (slide 35). `cudaDeviceSynchronize()` makes the host wait. L07
slide 33 uses `cudaThreadSynchronize()`, the old name of the same function
(see section 7). A `cudaMemcpy` from the device also waits for earlier
kernels to finish first.

### 3.6 Matrix multiplication on the GPU (L07 slides 27–34)

The CPU version needs three nested loops: N × N × N multiply-adds.
Matrices are stored **row by row** ("row-major", slide 28): element
(row, col) of an N × N matrix `m` is `m[row * N + col]`.

```c
// a = b * c on the CPU
for (int i = 0; i < N; i++)            // row of a
    for (int j = 0; j < N; j++) {      // column of a
        float sum = 0;
        for (int k = 0; k < N; k++) sum += b[i * N + k] * c[k * N + j];
        a[i * N + j] = sum;
    }
```

Every element `a[i][j]` can be computed on its own: it only reads `b` and
`c`, and nobody else writes `a[i][j]`. So the GPU version removes the two
outer loops and gives **one thread per element of a**. With a 2-D grid of
2-D blocks:

```c
__global__ void matmul(float *a, const float *b, const float *c, int N) {
    int col = blockIdx.x * blockDim.x + threadIdx.x;   // x picks the column
    int row = blockIdx.y * blockDim.y + threadIdx.y;   // y picks the row
    if (row < N && col < N) {
        float sum = 0.0f;                              // in a register: fast
        for (int k = 0; k < N; k++)
            sum += b[row * N + k] * c[k * N + col];
        a[row * N + col] = sum;
    }
}

dim3 block(16, 16);                                    // 256 threads per block
dim3 grid((N + 15) / 16, (N + 15) / 16);               // enough blocks to cover N x N
matmul<<<grid, block>>>(d_a, d_b, d_c, N);
```

L07 does the same with 1-D blocks of 32 threads and `row = blockIdx.y`
(slides 30–31): for N = 1024 the grid is 32 × 1024 blocks of 32 threads,
which is 1,048,576 threads, one per element. Slide 11 reports speedups of
over 200 times against one CPU core for this kind of code.

No locks are needed: each thread writes a different element of `a`, and
`b` and `c` are only read.

### 3.7 Synchronization on the GPU

GPU threads that share data have the same problems as CPU threads.

**Barrier inside a block.** `__syncthreads()` makes every thread of the
block wait until all of them have reached it (slide 41). Use it between
"everyone writes to shared memory" and "everyone reads what the others
wrote". It does **not** wait for threads in other blocks. The only way to
wait for the whole grid is to end the kernel and start another one.

Every thread of the block must reach the same `__syncthreads()`. Putting
it inside an `if` that only some threads enter is a bug: the program can
hang or give wrong answers.

**Atomics.** If many threads update one variable, `*total += x[i]` is a
race condition, exactly like `counter++` in module 01: read, add, write,
with other threads slipping in between. CUDA's atomic functions (slide 42)
do the read-modify-write as one indivisible step:

```c
atomicAdd(total, x[i]);     // safe, but all threads queue up for the same address
```

| Function | Effect (done as one step) |
|---|---|
| `atomicAdd(p, v)` / `atomicSub(p, v)` | `*p = *p + v` / `*p = *p - v` |
| `atomicMax(p, v)` / `atomicMin(p, v)` | keep the larger / smaller |
| `atomicExch(p, v)` | `*p = v`, return the old value |
| `atomicCAS(p, expected, v)` | if `*p == expected` then `*p = v`; return the old value (compare-and-swap, module 06) |

Each atomic call returns the *old* value. Atomics are correct but slow when
thousands of threads hit the same address, because they are done one
after another. Section 8.4 shows the fast way: add up inside each block
first.

## 4. Why it works (and when it doesn't)

**Why the GPU is fast here.** The matrix and vector kernels have
*independent* work items: no item reads what another item writes. So the
order in which threads run does not matter, and no locks are needed. The
GPU can run tens of thousands of threads, and while some wait for memory,
others compute. That hides the slow memory.

**When the GPU loses** (slide 14):

- **Little work per byte copied.** Adding two vectors does 1 addition per
  12 bytes moved. The copy over PCI Express takes longer than the CPU
  would need for the whole job. Matrix multiplication does N
  multiply-adds per element, so it is worth it.
- **Dependent steps.** `x[i] = x[i-1] + ...` is a chain. Thousands of
  cores can't speed up a chain.
- **Branchy code.** Threads that take different paths through `if`s slow
  each other down (section 8.1).
- **Shared counters.** Many threads updating one variable queue up on the
  atomic.

**Why SIMD is safe for the vector add.** Lane k only touches element
i + k. No lane reads what another lane writes, so doing them in the same
instruction gives exactly the same result as doing them one by one. For a
sum, the order *does* change, which is why floating-point sums need
`-ffast-math` or hand-written intrinsics.

## 5. Cheat-sheet

**AVX intrinsics** (`#include <immintrin.h>`, compile with `-mavx2`):

| Intrinsic | Effect |
|---|---|
| `__m256` | a register of 8 floats (`__m256d`: 4 doubles, `__m256i`: integers) |
| `_mm256_loadu_ps(p)` / `_mm256_storeu_ps(p, v)` | load / store 8 floats at any address |
| `_mm256_load_ps(p)` / `_mm256_store_ps(p, v)` | the same, but `p` must be a multiple of 32 |
| `_mm256_add_ps(a, b)` / `_mm256_sub_ps` / `_mm256_mul_ps` / `_mm256_div_ps` | 8 operations at once |
| `_mm256_fmadd_ps(a, b, c)` | a×b + c in one step (needs `-mfma`) |
| `_mm256_set1_ps(x)` / `_mm256_setzero_ps()` | 8 copies of x / 8 zeros |

**CUDA** (L07 slides 35–47):

| Call | Meaning |
|---|---|
| `__global__ void k(...)` | kernel: runs on the device, launched from the host |
| `k<<<blocks, threads>>>(args)` | launch; `blocks` and `threads` are `int` or `dim3` |
| `blockIdx`, `threadIdx`, `blockDim`, `gridDim` | built-in position variables (`.x`, `.y`, `.z`) |
| `cudaMalloc((void **)&d, bytes)` / `cudaFree(d)` | GPU memory |
| `cudaMemcpy(dst, src, bytes, cudaMemcpyHostToDevice / DeviceToHost)` | copy (destination first) |
| `cudaMemset(d, 0, bytes)` | fill GPU memory with a byte value |
| `cudaDeviceSynchronize()` | host waits for the GPU |
| `cudaGetLastError()`, `cudaGetErrorString(e)` | error checking |
| `__shared__`, `__constant__`, `__device__` (variables) | shared / constant / global memory |
| `__syncthreads()` | barrier for the threads of one block |
| `atomicAdd(p, v)` etc. | indivisible read-modify-write on global or shared memory |

## 6. Common exam mistakes

- Writing `blocks = n / threads`. It rounds **down** and the last elements
  are never computed. Round up: `(n + threads - 1) / threads`.
- Forgetting `if (i < n)` in the kernel.
- Using a host pointer inside a kernel, or a device pointer on the host.
  Copy with `cudaMemcpy` (destination first).
- Thinking `__syncthreads()` synchronizes the whole grid. It is only for
  one block.
- Returning a value from a `__global__` function. Kernels return `void`;
  results go into device memory.
- `*total += x[i]` in a kernel: a data race. Use `atomicAdd`, or a
  reduction.
- "The GPU is always faster." Not if the data copy costs more than the
  computation, or the work is a dependent chain.
- AVX question: saying "AVX uses more cores". It doesn't. One instruction
  does 8 operations on one core.
- AVX: forgetting the remainder loop when n is not a multiple of 8.
- Mixing up 8 floats and 4 doubles per 256-bit register.

## 7. Careful: what the notes say

- **Slide 38** names the block-index variable `blockID`. The real name is
  **`blockIdx`** (as the code on slides 26–31 uses).
- **Slide 33** uses `cudaThreadSynchronize()`. NVIDIA replaced it long ago
  with **`cudaDeviceSynchronize()`**; recent CUDA versions may warn or
  refuse the old name.
- **Slide 27** (CPU `mmult`) uses `a[...] += ...` and an empty `minit`.
  That is only correct if `a` starts as all zeros. The GPU kernel on
  slide 31 writes `a[...] = sum`, which is safer.
- **Slides 30–31** assume N is a multiple of 32 (no bounds check). For other
  N you need `if (row < N && col < N)` and rounded-up grid sizes, as in
  exercise 7.
- **Slide 11**: some matrix sizes in the table cannot be multiplied as
  written (the inner dimensions do not match). Read the table only for the
  trend: GPU speedups of 150–260 times.
- **Slide 37** writes the shared-memory size parameter type as `size_z`. It
  is `size_t`.
- **Slide 8**'s GPU column (1 FLOP per core per tick) is a double-precision
  figure. In single precision the same GPU peaks at about twice that
  (slide 4: 1.03 TFLOPS).

## 8. [EXTRA] Making GPU code fast

### 8.1 Warps and divergence

The GPU runs the threads of a block in groups of 32 called **warps**. The
32 threads of a warp execute the *same instruction at the same time*, like
the 8 lanes of an AVX register, only wider. NVIDIA calls this
*SIMT* (single instruction, multiple threads). `warpSize` (slide 38) is 32.

If threads of one warp take different sides of an `if`, the warp runs
**both** sides one after the other, and threads on the "wrong" side sit
idle. This is called **divergence**.

```c
if (threadIdx.x % 2 == 0) heavy_work();   // half the warp works, half waits
else                      other_work();   // then the other half works
```

Better: make whole warps take the same branch. For example,
`if ((threadIdx.x / warpSize) % 2 == 0)` sends warps 0, 2, 4, … one way
and warps 1, 3, 5, … the other way, so no warp is split. (Careful:
`if (threadIdx.x < 16)` still splits warp 0, because threads 0–15 and
16–31 are in the same warp.) Another fix is to sort the data so that
similar items sit next to each other.

### 8.2 Memory coalescing

When the 32 threads of a warp read 32 **neighbouring** addresses
(`x[i]` with consecutive `i`), the GPU fetches them in one or two big
memory transactions. This is called **coalescing**. If they read
scattered addresses (`x[i * 32]`), it needs up to 32 separate
transactions: up to 32 times slower. In the matrix kernel,
`c[k * N + col]` is coalesced (neighbouring threads have neighbouring
`col`), and `b[row * N + k]` is the same address for the whole warp, which
the hardware sends to all threads at once.

### 8.3 Shared-memory tiling for matrix multiplication

The simple kernel reads every element of `b` and `c` N times from slow
global memory. Tiling loads a 16 × 16 piece ("tile") into shared memory
once, and then all 256 threads of the block reuse it 16 times:

```c
#define TILE 16
__global__ void matmul_tiled(float *a, const float *b, const float *c, int N) {
    __shared__ float bs[TILE][TILE];       // a piece of b, shared by the block
    __shared__ float cs[TILE][TILE];       // a piece of c
    int tx = threadIdx.x, ty = threadIdx.y;
    int col = blockIdx.x * TILE + tx, row = blockIdx.y * TILE + ty;
    float sum = 0.0f;
    for (int t = 0; t < (N + TILE - 1) / TILE; t++) {       // tiles along the row of b / column of c
        int kb = t * TILE + tx, kc = t * TILE + ty;
        bs[ty][tx] = (row < N && kb < N) ? b[row * N + kb] : 0.0f;   // each thread loads ONE value
        cs[ty][tx] = (kc < N && col < N) ? c[kc * N + col] : 0.0f;
        __syncthreads();                    // the whole tile is loaded before anyone uses it
        for (int k = 0; k < TILE; k++) sum += bs[ty][k] * cs[k][tx];
        __syncthreads();                    // everyone is done before the tile is overwritten
    }
    if (row < N && col < N) a[row * N + col] = sum;
}
```

Both `__syncthreads()` calls are needed. Without the first, a thread may
read a tile slot that its neighbour has not loaded yet. Without the second,
a fast thread may load the *next* tile over values a slow thread is still
using. Notice that every thread reaches both barriers. The `if` only
decides *what* each thread loads, not *whether* it reaches the barrier.

### 8.4 Parallel reduction in shared memory

To add up n numbers, `atomicAdd` per element makes every thread queue for
one address. A **reduction** adds them in a tree instead. Inside one block
of 8 threads with values `3 1 4 1 5 9 2 6`:

```
part:      [3] [1] [4] [1] [5] [9] [2] [6]
stride 4:  threads 0-3 do part[t] += part[t+4]
           [8] [10] [6] [7]  .   .   .   .
stride 2:  threads 0-1 do part[t] += part[t+2]
           [14] [17]  .   .
stride 1:  thread 0 does part[0] += part[1]
           [31]                        -> 3+1+4+1+5+9+2+6 = 31
```

8 numbers take 3 rounds (log₂ 8). A block of 256 takes 8 rounds. Then
thread 0 of each block does **one** `atomicAdd` of its block's total, so
there are n/256 atomics instead of n. The code is in exercise 6:

```c
__shared__ int part[256];
int t = threadIdx.x, i = blockIdx.x * blockDim.x + t;
part[t] = (i < n) ? x[i] : 0;          // threads past the end add 0
__syncthreads();
for (int stride = blockDim.x / 2; stride > 0; stride /= 2) {
    if (t < stride) part[t] += part[t + stride];
    __syncthreads();                    // outside the if: every thread reaches it
}
if (t == 0) atomicAdd(total, part[0]);
```

This is the same tree as the MPI global sum in module 13, but with threads
in one block instead of processes.

## 9. No NVIDIA GPU? Use Google Colab

Google Colab gives you a free NVIDIA GPU in the browser (usually a T4).

1. Go to <https://colab.research.google.com> and create a new notebook.
2. Menu **Runtime → Change runtime type → Hardware accelerator: GPU (T4)**
   → Save.
3. Check the GPU in a code cell: `!nvidia-smi` (it should show a Tesla T4
   or similar) and `!nvcc --version`.
4. Get your code in. Either upload the `.cu` file with the folder icon on
   the left, or write it from a cell:
   ```
   %%writefile saxpy.cu
   ...paste your program here...
   ```
5. Compile and run: `!nvcc -o saxpy saxpy.cu && ./saxpy`
6. **If the kernel seems to do nothing** (all zeros, or every result
   unchanged), compile for the right GPU type. A T4 is "compute capability
   7.5": `!nvcc -arch=sm_75 -o saxpy saxpy.cu && ./saxpy`. With
   `CUDA_CHECK(cudaGetLastError())` after the launch you will see the
   reason instead of silent garbage.
7. To run this repo's tests on Colab, clone the repo in a cell
   (`!git clone https://github.com/dewminawijekoon/Concurrent_Programming_From_Zero.git`), then
   `!cd Concurrent_Programming_From_Zero && make test M=12`. Extra
   `nvcc` flags go in `NVCC_FLAGS`:
   `!cd Concurrent_Programming_From_Zero && NVCC_FLAGS=-arch=sm_75 make test M=12`.

Colab sessions are reset after a while; keep your `.cu` files on your own
machine.

## 10. How to answer "outline a CUDA solution" (exam, about 10 marks)

Markers look for these parts. Write them as short pseudocode plus one line
of explanation each.

1. **Decomposition:** what one thread computes (one element, or a range via a
   grid-stride loop) and why the pieces are independent.
2. **Kernel:** `__global__` header, the global-index line, the `if (i < n)`
   check, the per-thread work.
3. **Combining results:** if the answer is one number, say how partial
   results meet: `atomicAdd`, or a shared-memory reduction per block plus
   one `atomicAdd` per block, and why (avoid a race).
4. **Host code:** `cudaMalloc`, `cudaMemcpy` host→device, launch with
   `blocks = (N + T - 1) / T`, `cudaMemcpy` device→host, `cudaFree`.
5. **Synchronization and errors:** `__syncthreads()` where threads of a
   block share data; `cudaDeviceSynchronize()` / error checks after the
   launch.
6. **Performance remark** (often 1–2 marks): copy cost, coalesced access,
   divergence, load balance.

A full model answer is in the quiz answers (question 20).

## 11. Now do

| # | Exercise | Type | What you practise | Runs here without a GPU? |
|---|---|---|---|---|
| 1 | `exercises/concept/ex1_peak_flops.c` | warm-up | GFLOPS formula, SIMD lanes, copy cost | yes |
| 2 | `exercises/concept/ex2_avx_vector_add.c` | core [EXTRA] | AVX2 intrinsics with a remainder loop; speed table | yes (needs an AVX2 CPU) |
| 3 | `exercises/concept/ex3_autovec.c` | observation [EXTRA] | which loops the compiler vectorizes, and why | yes |
| 4 | `exercises/concept/ex4_thread_index.c` | core | grid/block/thread index arithmetic in plain C | yes |
| 5 | `exercises/concept/ex5_cuda_saxpy.cu` | core | the five host steps + a first kernel | needs CUDA (or Colab) |
| 6 | `exercises/concept/ex6_cuda_sum.cu` | core | race vs `atomicAdd` vs shared-memory reduction | needs CUDA |
| 7 | `exercises/concept/ex7_cuda_matmul.cu` | core | the L07 matrix kernel with 2-D grids and bounds checks | needs CUDA |
| 8 | `exercises/concept/ex8_cuda_vecadd_from_memory.cu` | from memory | a whole CUDA program from a blank page | needs CUDA |

```bash
make test M=12                        # CUDA tests say SKIP on machines without nvcc + an NVIDIA GPU
make test M=12 T=ex2_avx_vector_add   # one exercise (prints a scalar vs AVX speed table)
make run  M=12 EX=ex3_autovec         # shows which loops gcc vectorized
make run  M=12 EX=ex7_cuda_matmul ARGS=1000
```

Then do the [quiz](quiz.md) and check yourself with the
[recall sheet](recall.md). Answers are in
[`answers/12-simd-and-gpu/`](../../answers/12-simd-and-gpu/), but try
first.
