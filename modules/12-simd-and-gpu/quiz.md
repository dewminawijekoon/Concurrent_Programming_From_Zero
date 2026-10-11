# 12 · Quiz: SIMD and GPUs

Answer on paper first, as you would in the exam, then check
[`answers/12-simd-and-gpu/quiz-answers.md`](../../answers/12-simd-and-gpu/quiz-answers.md).
Marks in brackets show how much detail is expected. Tags: `[L07]` lecture,
`[EXTRA]` beyond the notes, `[tf]` true/false + justification, `[calc]`,
`[trace]`, `[design]`, `[code]`.

## A. True or false? Justify briefly. (2 marks each)

1. `[tf][EXTRA]` One AVX instruction on 256-bit registers can add 8 pairs of `double` values.
2. `[tf][L07]` A GPU is faster than a CPU because each GPU core is faster than a CPU core.
3. `[tf][L07]` A `__global__` function can return its result to the host with `return`.
4. `[tf][L07]` `__syncthreads()` makes every thread in the grid wait until all of them reach it.
5. `[tf][L07]` When `kernel<<<b, t>>>(...)` returns on the host, the kernel has finished.
6. `[tf][L07]` After `cudaMalloc((void **)&d_x, bytes)`, host code can read `d_x[0]`.
7. `[tf][EXTRA]` With `-O3`, gcc vectorizes every loop whose body is simple arithmetic.
8. `[tf][L07]` Moving a calculation to the GPU always makes it faster if the GPU's peak GFLOPS is higher.

## B. Calculations and traces

9. `[calc][EXTRA]` (3 marks) In exercise 3, `sum_array` (`s += x[i]`) is reported as "vectorized" by gcc at `-O3`, yet the machine code still uses `vaddss` (one float at a time). With `-ffast-math` it uses `vaddps` (8 at a time). Explain why gcc needs `-ffast-math` before it will add 8 at a time.
10. `[calc][L07]` (4 marks) A GPU has 2560 cores at 1.5 GHz, and each core does 2 FLOPs per clock tick. A CPU has 8 cores at 3.0 GHz, and each core does 32 FLOPs per tick (two 256-bit units doing fused multiply-adds). Compute both peaks in GFLOPS and the ratio. Explain where the CPU's 32 comes from.
11. `[calc][L07]` (4 marks) Multiplying two 4096 × 4096 float matrices takes 3.0 s on the CPU and 0.05 s in a GPU kernel. The two input matrices must be copied to the GPU and the result copied back, over a 12 GB/s bus (1 GB = 10⁹ bytes). What is the real speedup? What would it be if the copies were free?
12. `[calc][L07]` (3 marks) You process n = 5,000,000 elements with 256 threads per block. How many blocks do you launch? How many threads do no work? What goes wrong if you write `blocks = n / 256`?
13. `[trace][L07]` (3 marks) For `kernel<<<4, 8>>>`: (a) which global index does block 2, thread 5 get? (b) which block and thread handle element 30? (c) what are `gridDim.x` and `blockDim.x`?
14. `[trace][EXTRA]` (3 marks) A block of 8 threads does the shared-memory tree reduction of README section 8.4 on the values `2 7 1 8 2 8 1 8`. Write the contents of `part[0..3]`, `part[0..1]` and `part[0]` after each round.
15. `[trace][L07]` (3 marks) Two GPU threads run `*total += v` on the same `total` (initially 0); thread A has v = 3 and thread B has v = 5. List every possible final value of `*total` and give the order of steps for each. How does `atomicAdd` change the list?

## C. Explain and design

16. `[design][EXTRA]` (6 marks) Explain what AVX is and how it increases parallelism. Use a small example, and say what kind of loop cannot benefit from it.
17. `[design][L07]` (4 marks) Explain why a GPU is very good at matrix multiplication but would be a poor choice for following a linked list or for code full of `if` statements.
18. `[design][EXTRA]` (3 marks) A kernel does `if (i % 2 == 0) heavy(i); else light(i);`. Why can this run almost as slowly as if every thread called both functions? Suggest a fix.
19. `[design][EXTRA]` (3 marks) Kernel A has thread `i` read `x[i]`. Kernel B has thread `i` read `x[i * 32]`. Both read the same number of values. Which is faster on a GPU, and why?

## D. Code and outlines

20. `[code][L07]` (10 marks) **Outline a CUDA solution** that computes S = the sum of the decimal digits of every integer from 1 to N (for example, N = 12 gives 1+2+…+9 + (1+0) + (1+1) + (1+2) = 51), with N up to 10⁹, result printed by the host. Give the kernel and the host code in pseudocode, and explain how partial results are combined safely.
21. `[code][L07]` (6 marks) Write the kernel and the launch configuration (grid and block sizes) for C = A × B with N × N float matrices in row-major order, for any N (not only multiples of the block size).
22. `[code][EXTRA]` (4 marks) Using AVX intrinsics, write a function for `c[i] = a[i] * b[i] + c[i]` over n floats, where n need not be a multiple of 8 and the pointers need not be aligned.
