# 02 · Quiz: how much parallelism?

Answer on paper first, as you would in the exam, then check
[`answers/02-how-much-parallelism/quiz-answers.md`](../../answers/02-how-much-parallelism/quiz-answers.md).
Marks in brackets show how much detail is expected. Tags: `[L02]` lecture,
`[Lab01]` lab, `[tf]` true/false + justification, `[calc]` calculation
(show your working), `[design]`, `[proof]`.

Check your calculations with `make run M=02 EX=amdahl ARGS=...` (see the
top of `calc/amdahl.c` for all modes).

## A. True or false? Justify briefly. (2 marks each)

1. `[tf][L02]` A program that is 95% parallel can reach a speedup of 50 on 100 processors.
2. `[tf][L02]` In the lecture's ideal model, efficiency can be greater than 1.
3. `[tf][L02]` In Amdahl's law, p is the fraction of the program's lines of code that can run in parallel.
4. `[tf][L02]` If a program is 100% parallel, doubling the number of processors always halves its running time on a real machine.
5. `[tf][L02]` The parallelism T₁ / T∞ of a computation is an upper bound on its speedup, whatever the number of processors.
6. `[tf][L02]` Adding processors always improves both latency and throughput.
7. `[tf][L02]` To make a confidence interval half as wide, you need twice as many runs.
8. `[tf][L02]` The median of a set of running times is less affected by a few very slow runs than the mean.

## B. Calculations

9. `[calc][L02]` (3 marks) A program is 80% parallel. On 10 processors, what are its speedup and efficiency? What is the most it could ever gain with unlimited processors?

10. `[calc][L02]` (9 marks, exam style) Your program runs on 4 processors and is 75% parallel. You have two options for next year's budget:
    - **Option A:** buy a 12-processor machine and keep the program as it is.
    - **Option B:** keep 4 processors, but pay a programmer to make the program 90% parallel.

    (a) Compute the speedup of each option over one processor. (3)
    (b) Compute each option's limit with unlimited processors, and the speedup of Option B if you later also move to 8 processors. (3)
    (c) Which would you recommend, and why? (3)

11. `[calc][L02]` (3 marks) A program is 90% parallel. What is the smallest number of processors that gives a speedup of at least 5? Is a speedup of 10 possible?

12. `[calc][Lab01]` (5 marks) Lab 01 Q4.2–4.3 style. With 1 thread, the serial phase of a program takes 3 s and the parallel phase takes 9 s. With 4 threads the whole program takes 5.6 s.
    (a) Estimate p from the 1-thread phases. (b) Estimate p from the measured speedup with 4 threads. (c) Using (a), predict the speedup on 64 processors and on unlimited processors. (d) Why might (a) and (b) differ?

13. `[calc][L02]` (5 marks) A job has six tasks, with times in seconds: A (1), B (2), C (3), D (1), E (4), F (2). The arrows show which task must finish before which starts:
    ```
    A -> B,  A -> C,  B -> D,  C -> D,  C -> E,  D -> F,  E -> F
    ```
    Compute the work, the span (give the critical path), and the parallelism. What is the smallest possible running time on 2 processors, by the work and span laws?

14. `[calc][L02]` (4 marks) List the strands of P-FIB(3) and compute its work and span. Then give the work, span and parallelism of P-FIB(5) (you may use the recurrences from the lesson).

15. `[calc][L02]` (3 marks) A response time has mean 50 ms and standard deviation 4 ms. How many runs do you need for ± 2% accuracy at 95% confidence? At 99%?

16. `[calc][L02]` (3 marks) Four runs take 12, 10, 11 and 15 s. Give the mean, the sample standard deviation and the 95% confidence interval for the true mean.

## C. Explain and design

17. `[L02]` (4 marks) Name four reasons why the efficiency of a parallel program is usually below 1, with one example each.

18. `[L02]` (3 marks) Should you measure latency or throughput for: (a) the time a game takes to draw one frame; (b) a job that processes a million photos overnight; (c) a web server under heavy load? Justify each in one line.

19. `[Lab01]` (4 marks) Lab 01 Ex 4: a program sorts 10 million numbers with `qsort` (serial) and then sums them in N chunks (parallel). Our measurement: sort 1.06 s, sum 0.006 s with 1 thread. (a) Estimate p. (b) Why do the speedups barely change from N = 1 to N = 8? (c) Suggest one way to increase p (Q4.4).

20. `[L02]` (4 marks) A classmate's graph of their Lab 01 results has: number of threads on the y-axis and time on the x-axis; no units; no title; one run per point; three coloured lines with no key. List four problems and how to fix them.

21. `[design][L02]` (3 marks) Your parallel program reaches a speedup of only 3 on 8 cores. Suggest two measurements that would help you find out why.

## D. Proof

22. `[proof][L02]` (4 marks) Prove the work law and the span law, and use them to show that speedup can never exceed the parallelism T₁ / T∞.

23. `[EXTRA]` (3 marks) Amdahl's law says 90% parallel on 10 processors gives 5.26. Gustafson's law gives 9.1 for the same numbers. Explain how both can be right.
