# 01 · Quiz: races and interleavings

Answer on paper first, as you would in the exam, then check
[`answers/01-races-and-interleavings/quiz-answers.md`](../../answers/01-races-and-interleavings/quiz-answers.md).
Marks in brackets show how much detail is expected. Tags: `[L01]`/`[L03]`
lecture, `[Lab01]` lab, `[tf]` true/false + justification, `[trace]` give an
order of steps, `[output]` "is this output possible?", `[calc]`, `[design]`,
`[proof]`, `[code]`.

Unless a question says otherwise: every line is one atomic step, except
that `x = x + 1`, `x++` and `x = y + z` on shared variables are split into
separate loads and stores ("load-store machine"). The scheduler is fair.

## A. True or false? Justify briefly. (2 marks each)

1. `[tf][L03]` On a multicore computer, `counter++` on a shared `int` is a single atomic step.
2. `[tf][L03]` Two threads each run `counter++` once, starting from 0. The final value can be 0.
3. `[tf][Lab01]` In Lab 01 Ex 2 (N threads each increment an unprotected counter M times), the final value can be larger than N × M.
4. `[tf][L03]` A program that gave the right answer in 10,000 test runs has no race condition.
5. `[tf][L03]` Two processes with 3 atomic steps each have 20 possible interleavings.
6. `[tf][L03]` L03 slide 36: `P1: continue = false;` and `P2: while (continue);` run on **one** processor. The scheduler gives the processor to a process until it finishes or blocks. This scheduler is fair.
7. `[tf][L01]` In L01's prime example, splitting 1..10¹⁰ into ten equal blocks gives each thread the same amount of work.
8. `[tf][L01, L03]` A lost update is a violation of a safety property.
9. `[tf][Lab01]` Protecting every increment with one global mutex makes the Lab 01 counter program faster, because the threads no longer interfere.

## B. Traces, outputs and counting

10. `[trace][Lab01]` (4 marks) Lab 01 Q2.2: explain at the level of load, add and store how `counter++` in two threads can lose an update. Draw a timeline (a table with one row per step) for an interleaving that loses one update.

11. `[calc][L03]` (4 marks) (a) How many interleavings do 3 processes with 2 atomic steps each have? (b) 2 processes with 4 steps each? (c) Explain in words why the formula divides by (m!)ⁿ.
    Check: `make run M=01 EX=interleavings ARGS="3 2"`.

12. `[output][L03]` (4 marks) L03 slide 41, with `y = 0, z = 0`:
    ```
    P1:  x = y + z;          P2:  y = 1;  z = 2;
    ```
    On a load-store machine, list every possible final value of x, with an order of steps for each. What changes if the compiler loads `z` before `y`?

13. `[output][L03]` (4 marks) L03 slide 35, with `c1 = 2, c2 = 3`:
    ```
    P1:  c1 = c1 * c2;       P2:  c1 = c1 + c2;
    ```
    (a) If each line is one atomic step, what are the possible final values of c1? (b) On a load-store machine (load c1, compute, store c1), what are they? Give an order for each new value.

14. `[output]` (3 marks) Is output `P Q` possible? Can the program print nothing at all?
    ```
    shared a = 0, b = 0
    thread P:  a = 1;  if (b == 0) print P
    thread Q:  b = 1;  if (a == 0) print Q
    ```

15. `[output]` (5 marks) Each thread prints letters as it runs:
    ```
    shared x = 0
    thread A:                 thread B:
        x = 1                     if (x == 1) {
        print A                       x = 2
        if (x == 2) print A           print B
                                  }
                                  print B
    ```
    (a) Is `A A B` possible? (b) Is `B A A B` possible? (c) Is `B B A` possible? (d) List every possible output that starts with `B`.

16. `[output]` (4 marks) Both threads loop forever:
    ```
    shared x = 0
    thread A:  loop { x = 1; print A }
    thread B:  loop { if (x == 1) { x = 0; print B } }
    ```
    With a fair scheduler, can the program print (a) `A B A B ...` forever? (b) `A A A ...` forever? (c) `A A B A A B ...` forever? Justify each.

17. `[output]` (5 marks) Two threads each run `n++` **twice** (load and store separate), starting from `n = 0`. List every possible final value of n. Give the order of steps for the smallest one.

## C. Lab-style analysis

18. `[Lab01]` (3 marks) Lab 01 Q2.3: does increasing the number of threads N make the lost-update problem worse? What would you expect on a 4-core machine for N = 2, 4, 8, 16, and why?

19. `[Lab01]` (4 marks) Lab 01 Q3.1–3.2: Part A locks a global mutex for every increment. Part B counts in a local variable and locks once per thread. Why is Part B so much faster? Use the words *lock contention* in your answer. How many times is the lock taken in each part for N = 8, M = 100,000?

20. `[Lab01]` (2 marks) Lab 01 Q3.3: what happens if a thread forgets to unlock the mutex after its critical section?

21. `[design][L01]` (4 marks) In the prime example, the dynamic version hands out **one** number per call to `getAndIncrement()`. (a) Why can this be slow with many threads? (b) Suggest a change that keeps the load balanced but makes the shared counter less busy.

22. `[L01, L03]` (3 marks) Define *safety* and *liveness*. Classify: (a) the counter never ends lower than N × M; (b) every thread that calls `lock()` eventually gets the lock; (c) no deadlock. Note any disagreement between L01 and L03.

## D. Code and proof

23. `[code][L01]` (4 marks) Write a thread-safe `long get_and_increment(struct counter *c)` in C, (a) with a `pthread_mutex_t`, and (b) with a C11 atomic. Say which steps the original two-line version can interleave badly.

24. `[proof]` (3 marks) Explain why, with every `counter++` inside the same mutex, N threads that each increment M times always end with exactly N × M.

25. `[code]` (3 marks) This function is called by many threads. Find the race and fix it.
    ```c
    int seats_left = 10;
    pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
    int book_seat(void) {
        pthread_mutex_lock(&m);
        int ok = seats_left > 0;
        pthread_mutex_unlock(&m);
        if (ok) {
            pthread_mutex_lock(&m);
            seats_left--;
            pthread_mutex_unlock(&m);
        }
        return ok;
    }
    ```
