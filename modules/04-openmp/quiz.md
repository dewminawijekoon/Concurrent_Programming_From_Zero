# 04 · Quiz: OpenMP

Answer on paper first, as you would in the exam, then check
[`answers/04-openmp/quiz-answers.md`](../../answers/04-openmp/quiz-answers.md).
Marks in brackets show how much detail is expected. Tags: `[L04]` lecture,
`[Lab03]` lab, `[EXTRA]` beyond the slides, `[tf]` true/false +
justification, `[output]`, `[trace]`, `[calc]`, `[design]`, `[code]`.

## A. True or false? Justify briefly. (2 marks each)

1. `[tf][L04]` If you compile an OpenMP program without `-fopenmp`, gcc stops with an error.
2. `[tf][L04]` `#pragma omp parallel num_threads(8)` always starts exactly 8 threads.
3. `[tf][L04]` A variable declared inside a parallel region is shared by all threads of the team.
4. `[tf][EXTRA]` `#pragma omp atomic` can protect a block of several statements.
5. `[tf][EXTRA]` With `schedule(static)`, each iteration runs on the same thread in every run of the program.
6. `[tf][EXTRA]` With `reduction(+:s)`, the threads add to `s` one at a time, under a lock, in every iteration.
7. `[tf][EXTRA]` `#pragma omp parallel` directly followed by a `for` loop (no `for` in the pragma) shares the iterations among the threads.
8. `[tf][EXTRA]` Like `single`, `master` makes the other threads wait at the end of its block.

## B. Short answer

9. `[output][L04]` (3 marks) A parallel region with `num_threads(3)` prints `hello from thread %d of %d`. Which of these outputs are possible? (a) threads 0, 1, 2 in that order; (b) 2, 0, 1; (c) 0, 0, 1. Why?
10. `[output][Lab03]` (5 marks) What does each program print? Justify.
    ```c
    int x = 10;
    #pragma omp parallel num_threads(3) firstprivate(x)
    { x += omp_get_thread_num(); }
    printf("%d\n", x);                              // (a)

    int y = 10;
    #pragma omp parallel num_threads(3)
    {
    #pragma omp atomic
        y += omp_get_thread_num();
    }
    printf("%d\n", y);                              // (b)
    ```
    (c) Program (b) without the `atomic` line: list every value it can print. Check with `demo/shared_update.il`.
11. `[calc][EXTRA]` (4 marks) A loop has 10 iterations and runs on 4 threads with GCC. Which thread runs iteration 7 with (a) `schedule(static)`; (b) `schedule(static, 2)`? Give the whole mapping for both.
12. `[design][Lab03]` (4 marks) Choose a schedule and justify: (a) the 3×3 box blur of a 2000×2000 image; (b) counting the primes below 10 million, one number per iteration; (c) one Mandelbrot row per iteration, where some rows take 100 times longer than others.
13. `[Lab03]` (4 marks) Lab 03 Q2.1 and Q2.2: rank Monte Carlo pi with `critical`, `atomic`, `reduction` and per-thread counters from fastest to slowest, and explain. How does `reduction` relate to Lab 01 Ex 3 Part B?
14. `[Lab03]` (3 marks) Lab 03 Q4.3: why is `#pragma omp single` needed before the first call of the parallel merge sort? What goes wrong without it?
15. `[Lab03]` (3 marks) Lab 03 Q4.2: what happens if the parallel merge sort creates tasks all the way down to single elements? Use the word *granularity*.
16. `[Lab03]` (3 marks) Lab 03 Q1.1: inside a region with `private(x)`, what value does `x` have when a thread starts? What value does `x` have after the region? What changes with `firstprivate(x)`?
17. `[trace][EXTRA]` (4 marks) Compute the parallel prefix sum of `2 4 1 3 5 1` with 3 threads using the two-pass method of exercise 2. Show pass 1, the block totals, the offsets, and the result.

## C. Can this loop run in parallel?

18. `[code][EXTRA]` (8 marks) For each loop, say whether `#pragma omp parallel for` is safe as written. If not, explain the problem and fix it if possible.
    ```c
    (a) for (i = 0; i < n; i++) a[i] = b[i] + c[i];
    (b) for (i = 1; i < n; i++) a[i] = a[i-1] + 2;          // a[0] = 1
    (c) for (i = 0; i < n; i++) { t = a[i]; a[i] = b[i]; b[i] = t; }   // t declared before the loop
    (d) for (i = 0; i < n - 1; i++) a[i] = a[i+1] + 1;
    (e) for (i = 0; i < n; i++) p = f(p, i);                // f is a general function
    (f) for (i = 0; i < n; i++) if (a[i] > best) best = a[i];
    ```
19. `[calc][EXTRA]` (3 marks) Find a formula for `a[i]` that uses only `i`, and say why it makes the loop parallel: `a[0] = 1; a[i] = a[i-1] + 2*i + 1`.

## D. Code and design

20. `[code][L04]` (6 marks) Write the trapezoidal rule with OpenMP the lecture's way: a parallel region, each thread computes its own block of trapezoids, and a critical section. Then explain why there is one critical section per thread and not one per trapezoid.
21. `[code][EXTRA]` (4 marks) Rewrite question 20 as one `parallel for` loop with `reduction`.
22. `[code][Lab03]` (6 marks) Write `merge_sort_parallel` with tasks and a depth cutoff, including how it is started. Mark where `taskwait` goes and why.
23. `[design][Lab03]` (3 marks) Why must Monte Carlo pi use `rand_r(&seed)` with a seed declared inside the parallel region, instead of `rand()`?
