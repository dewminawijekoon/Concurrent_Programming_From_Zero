# 00 · Quiz answers: the big picture

These answers are about the length a marker expects for the marks shown.
"Why" is always part of the answer.

## A. True / false

1. **False.** Moore's law is about the number of **transistors** on a chip
   doubling about every two years (L01 slide 2). Clock speed stopped rising
   in the mid-2000s, because faster clocks made chips too hot.
2. **False.** A single-threaded program uses only one core, however many
   there are. Extra cores sit idle unless the program is split into parts
   that run at the same time (L01 slides 4–6).
3. **False.** Concurrent means several tasks are *in progress* at the same
   time. On one core they take turns, so nothing runs at the same instant.
   Parallel means they really run at the same instant on different cores.
4. **True.** The operating system switches quickly between the threads on
   the one core (L03 calls this pseudo-concurrent execution). The program
   must be correct for every order of steps, whether on one core or many.
5. **False.** Each core has its own program counter and runs its own
   instruction stream on its own data, so it is **MIMD** (L01 slide 15).
6. **True.** One instruction is applied to many data items at once, which
   is the definition of SIMD (L01 slide 15 lists GPUs under SIMD).
7. **False.** This is asynchrony (L01 slide 12): a thread can be paused at
   any moment by a cache miss, a page fault or the scheduler. A correct
   program must not depend on which thread is faster.
8. **False.** Each process has its own private memory, so there are no
   shared variables. Processes must send messages (L10 slide 2; MPI in
   module 13).
9. **False.** The scheduler decides when each thread runs. The order changes
   from run to run (Lab 01 Q1.1). Creating a thread first does not make it
   run first.

## B. Short answer

10. **Concurrent:** several tasks are in progress at the same time and may
    take turns on one core. Example: a text editor that checks spelling
    while you type. **Parallel:** several tasks really run at the same instant
    on different cores and cooperate on one problem. Example: four threads
    each summing a quarter of an array. **Distributed:** separate programs,
    usually on different machines, cooperate by sending messages. Example:
    a weather model running on 100 computers of a cluster (L01 slide 11).
11. Making one core faster meant a faster clock, which used much more power
    and made the chip too hot to cool. But transistors kept getting smaller,
    so makers used the extra transistors for more cores instead. For
    programmers the "free ride" is over: a program does not get faster on a
    new machine unless it is written to use several cores at once. That means
    splitting the work and coordinating the parts correctly (L01 slides 4–7).
12. (a) **SISD:** one instruction stream, one data item at a time. (b)
    **MIMD:** 8 cores, each with its own instruction stream working on its
    own data (a different web request). (c) **SIMD:** the same shading
    instruction is applied to many pixels at once. (d) **MISD:** several
    instruction streams work on the *same* data, so that a fault in one can
    be outvoted (L01 slide 15, "fault tolerant schemes").
13. Asynchrony means a thread can be delayed at any moment, for an unknown
    time, without warning (L01 slide 12). Causes: a cache miss (short), a page
    fault (long), the scheduler pausing the thread when its time slice is used
    up (very long). The rule: **never assume anything about the relative speed
    of threads.** The program must be correct for every possible timing.
14. | | Shared memory | Distributed memory |
    |---|---|---|
    | communicate by | reading and writing the same variables | sending messages |
    | main danger | races: two threads update the same variable at once | slow messages; waiting for a message that never comes |
    | C tool | Pthreads, OpenMP | MPI |
15. Any two: some problems are too big for one processor, such as weather
    prediction or rendering a film (L01 slide 13); many real-world problems are
    naturally parallel, such as traffic or an ant colony; programs often wait
    for input and output, and other work can run meanwhile (slide 14); cloud
    computing makes many machines easy to get (slide 14).

## C. Think about it

16. (a) **No.** Bigger numbers are harder to test (trial division needs more
    divisions), and the ranges do not contain the same number of primes, so
    the threads have different amounts of work (L01 slide 19). The last core
    has the most work, and everyone must wait for it. This is called **load
    imbalance**. Exercise 2 shows it: with 4 threads up to 2,000,000, the
    last thread does more than twice the work of the first. (b) Use **dynamic
    load balancing**: keep a shared counter, and let each thread repeatedly
    take the next number (or the next small batch of numbers) from it. A
    thread that finishes early simply takes more. The counter must be
    updated atomically. Module 01 shows why (L01 slides 20–26).
17. Every thread receives the **same address**, `&i`. By the time a thread
    reads `*arg`, the loop may already have changed `i`, so several threads
    can see the same index, or even the final value of `i`. This is a race on
    `i`. Fix: pass the value itself, `(void *)(long)i`, and read it with
    `(int)(long)arg`. Or give each thread its own array element:
    `ids[i] = i; pthread_create(..., &ids[i]);`.
18. When `main` returns, the whole process ends, and all its threads are
    killed, even if they have not finished. Some lines may never be printed,
    and the results are lost (Lab 01 Q1.2). `pthread_join` makes `main` wait.

## D. Code

19. ```c
    void *worker(void *arg) {
        int i = (int)(long)arg;
        printf("thread %d\n", i);
        return (void *)(long)(i * i);
    }
    // in main:
    void *res;
    pthread_join(t[i], &res);
    long square = (long)res;
    ```
    Marks: getting `i` back with the right casts (1), printing (1), returning
    through `void *` (1), joining and casting the result back (1).
