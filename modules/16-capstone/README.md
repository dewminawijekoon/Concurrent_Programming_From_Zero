# 16 · Capstone: one problem, four paradigms ★

| | |
|---|---|
| **Time** | about 5 hours |
| **You should know first** | modules 03 (Pthreads), 04 (OpenMP), 12 (CUDA), 13 (MPI); module 11 helps |
| **Lectures** | all, especially L01 slides 17–26 (prime printing and load balancing), L04, L07, L10 |
| **Helps with** | the exam's last question: "solve this problem with Pthreads / OpenMP / MPI / CUDA", plus "justify your load-balancing scheme" (about 5 marks) |

**Goal:** take one problem with uneven work per item, solve it in all four
paradigms, *measure* how well the work is spread, and write exam-style
outlines and justifications.

Words in **bold** link to the [glossary](../../GLOSSARY.md) the first time
they appear. There is no lab for this module.

---

## 1. The problem

**Count the twin-prime pairs up to N.** A twin-prime pair is two primes
that differ by 2: (3, 5), (5, 7), (11, 13), (17, 19), (29, 31), … There are
8 pairs with both numbers ≤ 100, and 8169 up to one million.

We test every p = 0, 1, …, N − 2 (we call each p an **item**): is p prime,
and if so, is p + 2 prime? Primality is tested by **trial division**: try
dividing by 2, then by 3, 5, 7, … up to √p. (This is deliberately simple,
like the `isPrime` of L01 slide 18.)

All items are independent, so this is easy to split between workers. The
interesting part is that **items cost very different amounts of work**.
Work is counted in division tests (`twins.h`), so it is the same on every
machine:

| item p | what happens | work |
|---:|---|---:|
| 1000 | even: rejected by the first test | 1 |
| 101 | prime; then 103 is prime too: a pair | 10 |
| 999 983 | prime (500 tests); 999 985 ends in 5, rejected after 3 tests | 503 |
| 999 959 | prime, and 999 961 is prime: a pair | 1000 |

So:

- **Even numbers cost 1.** Half of all items are almost free.
- **Primes cost about √p / 2** tests, which **grows** with p.
- Primes become **rarer** as p grows (about 1 in ln p numbers is prime),
  but each one costs more. Overall the work per item still grows along the
  range: the last quarter of the items costs more than twice the first
  quarter.

This is the same situation as L01 slides 17–19: "higher ranges have fewer
primes, yet larger numbers are harder to test", so an even split of the
*range* is not an even split of the *work*.

## 2. The idea: how to share uneven work

### 2.1 Measuring imbalance

A parallel job finishes when its **slowest** worker finishes. So we
measure:

```
imbalance = (largest work of any worker) / (average work per worker)
```

1.00 is perfect. 1.32 means the slowest worker does 32% more than its
share: the others sit idle for part of the time, and the speedup can be at
most T / 1.32 instead of T. Exercise 1 computes it.

### 2.2 Four strategies

| Strategy | Who does item p | Measured imbalance, N = 10⁶, 4 workers |
|---|---|---:|
| **block** | worker t gets one run of neighbouring items | 1.32 |
| **cyclic** | worker p % T (deal like cards) | **1.98** |
| **block-cyclic**, blocks of 1000 | worker (p / 1000) % T | 1.00 |
| **dynamic**, chunks of 64 | whoever is free takes the next chunk from a shared counter | about 1.03 |

**Block** gives the last worker the largest numbers, which are the most
expensive. That is the L01 problem.

**Cyclic surprised us: it is the worst.** With 4 workers, worker 0 gets
p = 0, 4, 8, …, and worker 2 gets p = 2, 6, 10, …: **only even numbers**,
each rejected after one test. Workers 1 and 3 get all the odd numbers and
do almost all the work. This happens whenever the number of workers is
even. With other worker counts, other patterns appear: with 3 workers,
every prime p that leaves remainder 1 when divided by 3 has a p + 2 that is
divisible by 3, so the expensive second test always lands on the same
worker (imbalance 1.61). With 6 workers it reaches 3.20. Lesson: **a fixed
round-robin split can line up with patterns in the data.**

| workers | 2 | 3 | 4 | 5 | 6 | 7 | 8 |
|---|--:|--:|--:|--:|--:|--:|--:|
| block | 1.23 | 1.29 | 1.32 | 1.33 | 1.34 | 1.35 | 1.35 |
| cyclic | 1.97 | 1.61 | 1.98 | 1.28 | 3.20 | 1.18 | 1.98 |

**Block-cyclic** deals blocks of 1000 neighbouring items round-robin. Each
block contains both even and odd numbers, and every worker gets blocks from
the whole range. That breaks both patterns.

**Dynamic** does not decide in advance. Workers take the next chunk when
they are free, so a worker that got expensive items simply takes fewer
chunks. This is L01's **shared counter** (slides 20–26): the
`getAndIncrement` must be atomic, which is exactly an atomic fetch-and-add.

### 2.3 The chunk size trade-off

For dynamic scheduling, the chunk size matters:

- **Too small** (1 item): every item needs one atomic operation on the
  shared counter. All workers fight over one memory location (and, in MPI,
  one message round trip per chunk). Overhead.
- **Too large** (say N / T items): back to a static split, with its
  imbalance; at the end some workers sit idle while one finishes a big
  chunk.
- **In between** (here 64–1000): little overhead, good balance. OpenMP's
  `guided` schedule starts with big chunks and makes them smaller towards
  the end, to get both.

## 3. The mechanism: the same problem in four paradigms

The pseudocode below is the level of detail an exam outline needs. The
exercises contain the full C code.

### 3.1 Pthreads (exercise 2)

```c
atomic_long next = 0;                      // shared counter for the dynamic version

void *worker(void *arg) {                  // one per thread
    job_t *j = arg;
    long count = 0, work = 0;              // PRIVATE: no race, no lock
    for (;;) {
        long start = atomic_fetch_add(&next, CHUNK);   // L01's getAndIncrement, atomically
        if (start >= M) break;
        for (long p = start; p < min(start + CHUNK, M); p++) count += tw_item(p, &work);
    }
    j->count = count;                      // read by main after pthread_join
    return NULL;
}
// main: pthread_create T workers; pthread_join all; total = sum of their counts
```

Key points: each thread counts **privately** and the results are combined
once after `pthread_join` (as in module 03's global sum). The only shared
variable is the atomic counter. Updating a shared `total` with `+=` inside
the loop would be a race; protecting it with a mutex would be correct but
slow.

### 3.2 OpenMP (exercise 3)

```c
long count = 0;
#pragma omp parallel for reduction(+:count) schedule(dynamic, 64)
for (long p = 0; p < M; p++)
    count += tw_item(p, &work_of_this_thread);
```

The `schedule` clause *is* the load-balancing decision:

| clause | means |
|---|---|
| `schedule(static)` | block (one run of items per thread) |
| `schedule(static, 1)` | cyclic |
| `schedule(static, 1000)` | block-cyclic |
| `schedule(dynamic, 64)` | shared counter, chunks of 64 |
| `schedule(guided)` | dynamic with chunks that shrink |

`reduction(+:count)` gives each thread a private copy and adds them at the
end (module 04). Exercise 3 uses `schedule(runtime)` so the test can try
every schedule.

### 3.3 MPI (exercise 4)

Static versions need no messages until the end:

```c
for (long p = my_rank; p < M; p += comm_sz) count += tw_item(p, &work);   // cyclic
MPI_Reduce(&count, &total, 1, MPI_LONG, MPI_SUM, 0, MPI_COMM_WORLD);
if (my_rank == 0) printf("%ld\n", total);
```

There is no shared memory, so there is no shared counter. Dynamic
balancing uses **manager–worker** instead:

```c
if (my_rank == 0) {                                   // manager
    next = 0; stopped = 0;
    while (stopped < comm_sz - 1) {
        MPI_Recv(&dummy, 1, MPI_INT, MPI_ANY_SOURCE, TAG_READY, comm, &status);
        if (next < M) { start = next; next += CHUNK; } else { start = -1; stopped++; }
        MPI_Send(&start, 1, MPI_LONG, status.MPI_SOURCE, TAG_WORK, comm);
    }
} else {                                              // worker
    loop: MPI_Send(&dummy, 1, MPI_INT, 0, TAG_READY, comm);
          MPI_Recv(&start, 1, MPI_LONG, 0, TAG_WORK, comm, MPI_STATUS_IGNORE);
          if (start < 0) stop;  else do items start .. start + CHUNK - 1
}
MPI_Reduce(&count, &total, 1, MPI_LONG, MPI_SUM, 0, comm);    // everyone, manager included
```

`MPI_ANY_SOURCE` lets the manager serve whichever worker is free first.
Each chunk costs one message round trip, so chunks must be bigger than in
Pthreads (here 1000). The manager computes nothing, so with 4 processes
the imbalance printed by exercise 4 is about 1.33: that is exactly the
cost of one idle process out of four, while the 3 workers are balanced.
With 32 processes the manager costs only 1/32 of the machine.

### 3.4 CUDA (exercise 5)

```c
__global__ void twins_kernel(long N, unsigned long long *total) {
    long first = blockIdx.x * blockDim.x + threadIdx.x;
    long step  = gridDim.x * blockDim.x;               // grid-stride loop: a fixed grid covers any N
    unsigned long long mine = 0;                       // private counter in a register
    for (long p = first; p <= N - 2; p += step)
        if (is_prime(p) && is_prime(p + 2)) mine++;
    if (mine) atomicAdd(total, mine);                  // one atomic per thread, not per item
}
// host: cudaMalloc(&d_total), cudaMemset(d_total, 0), launch, cudaMemcpy back, cudaFree
```

The grid-stride loop is a cyclic distribution over hundreds of thousands
of threads. The real problem on a GPU is **divergence** (module 12,
section 8.1): neighbouring threads of a warp test neighbouring numbers, so
in every warp some threads finish after 1 test while others run hundreds.
The warp is busy until its slowest thread is done. Improvements: skip even
numbers entirely (test only odd p), or let a block's threads share the
divisions of one number.

### 3.5 Comparing the four

| | Pthreads | OpenMP | MPI | CUDA |
|---|---|---|---|---|
| memory | shared | shared | separate per process | separate host / device |
| how work is split | you write it (block, cyclic, counter) | `schedule(...)` clause | rank arithmetic, or manager–worker | thread index, grid-stride loop |
| dynamic balancing | atomic shared counter | `schedule(dynamic)` / `guided` | manager–worker messages | hard; usually many small items per thread |
| combining results | private sums, then add after `pthread_join` | `reduction(+:count)` | `MPI_Reduce` | private sum + `atomicAdd` (or block reduction) |
| main danger | races on shared variables | forgetting `reduction` / private variables | mismatched messages, deadlock | divergence, forgetting the copy-back |

## 4. Why it works

- **Correctness** is the same argument in every version: every item is
  handled by **exactly one** worker, each worker counts privately, and the
  private counts are added once at the end. The tests check both: the
  count, and that the work adds up to the serial total (so no item is
  skipped or done twice).
- **Dynamic scheduling balances** because a worker only takes a new chunk
  when it has finished the last one. A worker that got expensive items
  takes fewer chunks. At the end the workers finish within about one chunk
  of each other. So the imbalance is roughly (one chunk of work) / (work
  per worker), which is tiny when there are many chunks.
- **Static block fails** because cost grows along the range; **static
  cyclic fails** here because the cost follows patterns (even/odd, remainders
  modulo 3) that a fixed round-robin can line up with.

## 5. Writing the exam answers

### 5.1 A load-balancing justification (about 5 marks)

Markers want: (1) why the work per item is uneven, for this problem; (2)
what goes wrong with the obvious split; (3) your scheme; (4) why it fixes
the problem; (5) its cost. For example:

> The cost of testing p by trial division grows with √p for primes, while
> even numbers and numbers with small factors are rejected almost at once.
> So the work per item is uneven, and grows along the range. A block split
> gives the last thread the most expensive numbers (we measured 32% more
> than average with 4 threads). A cyclic split with an even number of
> threads is worse: some threads receive only even numbers. I use dynamic
> scheduling: threads repeatedly take the next chunk of, say, 64 numbers
> from a shared counter (an atomic fetch-and-add; `schedule(dynamic, 64)`
> in OpenMP; manager–worker in MPI). A thread that gets expensive numbers
> simply takes fewer chunks, so all threads finish at about the same time.
> The chunk keeps the overhead of the shared counter small; one atomic
> operation per 64 numbers is negligible next to the divisions.

### 5.2 Outline checklist for each paradigm

- **Pthreads:** thread function, how it finds its items, private
  accumulation, how results are combined (join, or a mutex once per thread),
  what is shared and how it is protected.
- **OpenMP:** the `parallel for` with `reduction` and the `schedule` clause
  and a reason for the schedule.
- **MPI:** Init/size/rank, Bcast of N if only rank 0 reads it, the
  partition formula or manager–worker protocol, `MPI_Reduce` with all
  arguments, output on rank 0, Finalize (module 13 section 9).
- **CUDA:** kernel with the global index and grid-stride loop, private
  counter and `atomicAdd`, host steps (malloc, memset/copy, launch, check,
  copy back, free), and a remark on divergence (module 12 section 10).

## 6. Common exam mistakes

- "Each thread gets N / T numbers, so the work is balanced." Equal
  *numbers of items* are not equal *work*.
- Recommending cyclic without checking for patterns in the data.
- A shared `count++` (or `total += x`) inside the parallel loop without
  protection: a race. Use private counters.
- Dynamic scheduling with chunk size 1 in MPI: one message round trip per
  number.
- Forgetting to say how the partial results are combined.
- Saying the GPU solves load balancing because it has so many threads.
  Divergence inside warps is the GPU's version of load imbalance.

## 7. Careful: what the notes say

- **L01 slide 18** splits 1..10¹⁰ with `for (j = i*10^9+1, j < (i+1)*10^9; j++)`.
  The comma should be a semicolon, and `<` skips the last number of each
  range (10⁹, 2·10⁹, …); it should be `<=`.
- **L01 slides 21–22** test `while (j < 10^10)` *before* taking the next
  value from the counter. A thread can therefore take a value ≥ 10¹⁰ and
  still test (and print) it. Check the value right after taking it, as our
  exercises do (`if (start >= M) break;`).
- **L01 slide 19** says "higher ranges have fewer primes, yet larger numbers
  are harder to test" without saying which effect wins. For trial division
  the cost per prime (about √p) grows faster than the number of primes
  falls (about 1/ln p), so later blocks cost more: we measured
  5.7, 9.0, 11.0 and 12.5 million tests for the four blocks of 1..10⁶.

## 8. Now do

| # | Exercise | Type | What you practise | Runs here? |
|---|---|---|---|---|
| 1 | `exercises/concept/ex1_serial_and_partition.c` | warm-up | serial answer, block/cyclic rules, imbalance | yes |
| 2 | `exercises/concept/ex2_pthreads.c` | core | block vs cyclic vs dynamic with Pthreads; work and time tables | yes |
| 3 | `exercises/concept/ex3_openmp.c` | core | the same with `schedule(...)` clauses | yes (needs OpenMP) |
| 4 | `exercises/concept/ex4_mpi.c` | core | block, cyclic and manager–worker in MPI | needs MPI |
| 5 | `exercises/concept/ex5_cuda.cu` | core | grid-stride kernel with private counts and `atomicAdd` | needs CUDA (or Colab) |
| 6 | `exercises/concept/ex6_dynamic_from_memory.c` | from memory | the shared-counter version from a blank page | yes |

```bash
make test M=16                       # MPI and CUDA tests say SKIP if the tools are missing
make test M=16 T=ex2_pthreads        # prints the imbalance and time table
make run  M=16 EX=ex1_serial_and_partition ARGS="1000000 6"   # try 6 workers: look at cyclic!
make run  M=16 EX=ex4_mpi ARGS="2000000 dynamic"
make tsan M=16 T=ex2_pthreads        # private counters: no races expected
```

Then do the [quiz](quiz.md) and check yourself with the
[recall sheet](recall.md). Answers are in
[`answers/16-capstone/`](../../answers/16-capstone/), but try first.
