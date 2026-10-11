# 04 · Recall sheet

Cover the right-hand column. Say each answer out loud, or write it on a
blank page. If you hesitate, re-read that part of the [README](README.md).

| Question | What you should be able to say or write |
|---|---|
| What is OpenMP, and how do you switch it on? | A set of `#pragma omp` lines and `omp_...` functions for shared-memory parallel C. Compile with `gcc -fopenmp`; without it the pragmas are silently ignored. |
| What does `#pragma omp parallel` do? | The current thread starts a team; every thread runs the next block; at the end everyone waits (a barrier) and only the primary thread continues. |
| How does a thread find its number and the team size? | `omp_get_thread_num()` and `omp_get_num_threads()`. |
| Does `num_threads(p)` guarantee `p` threads? | No (L04 slide 50). Use `omp_get_num_threads()` for the real size. |
| Which variables are shared and which are private? | Declared before the region: shared. Declared inside: private. The loop counter of a `parallel for`: private. |
| What does `#pragma omp critical` do? | Only one thread at a time runs the next statement or block. |
| How does the lecture parallelise the trapezoidal rule? | Each thread adds up its own block of trapezoids in a private variable, then adds it to the shared total inside one critical section. |
| `parallel` vs `parallel for`? | `parallel`: every thread runs the whole block. `parallel for`: the loop's iterations are shared out, each runs once. |
| Name the schedules and when to use each. | `static`: equal blocks decided in advance, for equal-cost iterations. `dynamic, c`: free threads take the next `c` iterations, for uneven costs. `guided`: big chunks shrinking, uneven costs with less overhead. `auto`, `runtime`. |
| `private` vs `firstprivate` vs `lastprivate`? | `private`: own copy, no starting value. `firstprivate`: own copy starting as the outside value. `lastprivate`: after the loop, the outside variable gets the last iteration's value. |
| What does `reduction(+:s)` do? | Each thread gets a private `s` starting at 0; at the end the copies are added into the original `s`. Local accumulation done for you. |
| `critical` vs `atomic` vs `reduction`? | `critical`: a lock around a block. `atomic`: one simple update as one hardware step. `reduction`: private copies, combined once. Fastest to slowest for a sum: reduction, atomic, critical. |
| What is a loop-carried dependence? | An iteration needs a value written by an earlier iteration, like `a[i] = a[i-1] + i`. `parallel for` then gives wrong results. |
| How can you remove one? | A closed-form formula (`a[i] = i(i+1)/2`), a parallel prefix sum (two passes), or a reduction if only the total is needed. Some loops (`p = f(p, i)`) cannot be parallelised. |
| Describe the two-pass prefix sum. | Each thread scans its own block from 0; barrier; one thread turns the block totals into offsets; each thread adds its offset to its block. |
| `barrier`, `single`, `master`, `nowait`? | Barrier: all wait for all. Single: one thread runs it, the others wait at its end. Master: thread 0 runs it, no waiting. Nowait: drop the barrier at the end of `for` or `single`. |
| How do tasks work in merge sort? | `parallel` + `single` to start; in the recursion, a `task` for each half, `taskwait`, then merge; below a depth cutoff, sort serially. |
| Why the cutoff, and why `single`? | Tiny tasks cost more to manage than the work they do (granularity). Without `single`, every thread would start its own whole sort of the same array. |
| Why not `rand()` in parallel code? | It shares one hidden state between threads. Use `rand_r(&seed)` with a private seed per thread. |
| Why don't we use `make tsan` for OpenMP? | The OpenMP runtime is not built for ThreadSanitizer, so it reports races that are not real. |

**From memory:** write a `parallel for` + `reduction` sum and a
`reduction(max:...)` (exercise 5), the trapezoidal rule with `critical`,
and the task-based merge sort skeleton.
