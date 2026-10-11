# 00 · Quiz: the big picture

Answer on paper first, as you would in the exam, then check
[`answers/00-big-picture/quiz-answers.md`](../../answers/00-big-picture/quiz-answers.md).
Marks in brackets show how much detail is expected. Tags: `[L01]` lecture,
`[Lab01]` lab, `[tf]` true/false + justification, `[design]`, `[calc]`, `[code]`.

## A. True or false? Justify briefly. (2 marks each)

1. `[tf][L01]` Moore's law says that the clock speed of processors doubles about every two years.
2. `[tf][L01]` A single-threaded program runs about twice as fast on an 8-core machine as on a 4-core machine with the same cores.
3. `[tf][L01]` Every concurrent program is also a parallel program.
4. `[tf][L01]` A concurrent program can run correctly on a computer with only one core.
5. `[tf][L01]` A multicore CPU running four threads is an example of SIMD.
6. `[tf][L01]` A GPU that applies the same instruction to thousands of pixels at once is best described as SIMD.
7. `[tf][L01]` If thread A always started first in your tests, you may assume it always finishes its first step before thread B starts.
8. `[tf][L04, L10]` In a distributed-memory system, two processes can communicate by writing to the same global variable.
9. `[tf][Lab01]` After creating 8 threads that each print a line, the lines always appear in the order the threads were created.

## B. Short answer

10. `[L01]` (3 marks) Explain the difference between *concurrent*, *parallel* and *distributed* computing. Give one example of each.
11. `[L01]` (3 marks) Why did processor makers switch to multicore chips instead of making one faster core? What does this mean for programmers?
12. `[L01]` (4 marks) Classify each of these with Flynn's taxonomy and justify each in one line: (a) a 2005 single-core laptop running a word processor; (b) an 8-core server running a web server with 8 threads; (c) a graphics card shading pixels; (d) three flight computers that compute the same result and vote on it.
13. `[L01]` (3 marks) What is *asynchrony*? Give two causes of unexpected delay, and say what rule it gives us for writing concurrent programs.
14. `[L01, L10]` (4 marks) Compare shared memory and distributed memory: how do tasks communicate, what is the main danger in each, and which C tool from this course goes with each?
15. `[L01]` (2 marks) Give two reasons from the lecture for using parallel or concurrent computing, other than "computers have many cores now".

## C. Think about it

16. `[design][L01]` (4 marks) You must count the primes from 1 to 10¹⁰ on a 10-core machine. A friend gives each core one tenth of the range: core 0 gets 1 to 10⁹, core 9 gets the last 10⁹ numbers. (a) Will all cores finish at the same time? Explain. (b) Suggest a better way to share the work. (Run `make run M=00 EX=ex2_split_the_work` to see the effect on a smaller range.)
17. `[Lab01]` (3 marks) Lab 01 Ex 1 asks you to pass each thread its index through the `void *arg` parameter. A student writes `pthread_create(&t[i], NULL, worker, &i);` inside a `for` loop. What can go wrong, and how do you fix it?
18. `[Lab01]` (2 marks) What happens if `main` returns without calling `pthread_join` for the threads it created?

## D. Code

19. `[code][Lab01]` (4 marks) Write the body of a thread function `void *worker(void *arg)` that gets its index `i` from `arg`, prints it, and returns `i * i` to whoever joins it. Then write the line in `main` that collects this value from thread `t[i]`.
