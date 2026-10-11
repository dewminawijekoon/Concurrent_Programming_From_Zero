# 16 · Quiz: one problem, four paradigms

Answer on paper first, as you would in the exam, then check
[`answers/16-capstone/quiz-answers.md`](../../answers/16-capstone/quiz-answers.md).
Marks in brackets show how much detail is expected. Tags: `[L01]`, `[L04]`,
`[L07]`, `[L10]` lectures, `[EXTRA]`; `[tf]`, `[calc]`, `[design]`, `[code]`.

## A. True or false? Justify briefly. (2 marks each)

1. `[tf][L01]` If every thread gets the same number of items, the work is balanced.
2. `[tf][L01]` A cyclic distribution always balances better than a block distribution.
3. `[tf][L04]` `schedule(dynamic, 1)` is always the fastest OpenMP schedule for uneven work.
4. `[tf][L04]` `schedule(static, 1)` hands out the iterations cyclically.
5. `[tf][L01]` In the L01 shared-counter solution, `getAndIncrement` may be written as `temp = value; value = temp + 1; return temp;` with plain variables.
6. `[tf][L07]` On a GPU, uneven work per item costs nothing, because there are thousands of threads.

## B. Calculations

7. `[calc][L02]` (3 marks) Four threads do 10, 12, 14 and 20 units of work. What is the imbalance? What speedup over one thread do you get (ignore overheads)? What would perfect balance give?
8. `[calc][L01]` (3 marks) Items p = 0 .. 999 999 are split cyclically over 4 threads. Explain why threads 0 and 2 have almost no work. Which thread counts avoid this particular problem, and why does 6 make it even worse?
9. `[calc][L10]` (3 marks) MPI manager–worker with 4 processes and perfectly balanced workers. What is the best speedup over one process? With 32 processes? When is it worth letting the manager compute too?
10. `[calc][L04]` (3 marks) A loop has 10⁶ iterations; one atomic fetch-and-add on a shared counter costs 50 ns and the average iteration costs 400 ns. Estimate the overhead of `schedule(dynamic, 1)` and of `schedule(dynamic, 100)`, as a percentage of the useful work.

## C. Design and justification

11. `[design][L01]` (5 marks) Justify a load-balancing scheme for counting twin primes up to N with trial division on a 4-core machine.
12. `[design][L07]` (3 marks) Why does warp divergence hurt the CUDA version, and what simple change to the decomposition reduces it?
13. `[design][L10]` (3 marks) Why can't the MPI version use a shared counter like the Pthreads version? What does it use instead, and how should the chunk size differ?
14. `[design]` (4 marks) For each paradigm (Pthreads, OpenMP, MPI, CUDA), say how the partial counts are combined into one answer, and what goes wrong if you do it naively.

## D. Outlines (exam Q4 style)

15. `[code][L04]` (5 marks) Outline a Pthreads program for the twin-prime count with dynamic load balancing. Show the thread function, what is shared, and how the result is combined.
16. `[code][L04]` (3 marks) Give the OpenMP version in one loop, and explain each clause.
17. `[code][L10]` (8 marks) Outline the MPI version with manager–worker load balancing; the result must be printed by rank 0. Use the MPI functions correctly.
18. `[code][L07]` (6 marks) Outline the CUDA version: kernel and host code.
