# 06 · Quiz: n-thread locks and the hardware

Answer on paper first, as you would in the exam, then check
[`answers/06-locks-and-hardware/quiz-answers.md`](../../answers/06-locks-and-hardware/quiz-answers.md).
Marks in brackets show how much detail is expected. Tags: `[L06]` lecture,
`[tf]` true/false + justification, `[trace]` give a schedule, `[proof]`,
`[calc]`, `[code]`, `[design]`.

## A. True or false? Justify briefly. (2 marks each)

1. `[tf][L06]` The Filter lock for n threads uses n levels between the non-critical section and the critical section.
2. `[tf][L06]` The Filter lock is first-come-first-served.
3. `[tf][L06]` In the Bakery lock, two threads can never hold the same label at the same time.
4. `[tf][L06]` If the `max()` in the Bakery doorway is not atomic, the Bakery lock can let two threads into the critical section together.
5. `[tf][L06]` A starvation-free lock is always first-come-first-served.
6. `[tf][L08-TLP]` A test-and-test-and-set (TTAS) lock is fairer than a test-and-set (TAS) lock.
7. `[tf][L08-TLP]` False sharing happens when two threads keep writing the same variable.
8. `[tf][L09]` Busy waiting always wastes time, so a good program never spins.
9. `[tf][L08-TLP]` A test-and-set spin lock is starvation-free.
10. `[tf][L08-TLP]` A write-update coherence protocol needs less bus bandwidth than a write-invalidate protocol.

## B. Short answer

11. `[L06]` (3 marks) Explain the terms *doorway*, *bounded waiting* and *first-come-first-served*. Why is "starvation-free" not enough?
12. `[trace][L06]` (4 marks) Filter lock, n = 3, but only threads 0 and 1 try to enter. Give a numbered schedule in which thread 0 writes `level[0] = 1` before thread 1 does anything, yet thread 1 enters the critical section first.
13. `[proof][L06]` (4 marks) Explain why at most n − L threads can get past level L of the Filter lock. What does this give for L = n − 1?
14. `[L06]` (3 marks) Why does the Bakery lock compare (label, id) pairs instead of labels only? What goes wrong if the waiting condition is `flag[k] && label[k] < label[i]`? And with `<=`?
15. `[trace][L06]` (4 marks) A student removes `flag[]` from the 2-thread Bakery lock and uses "label 0 = not interested" instead (`unlock` sets `label[i] = 0`; the wait is `while (label[j] != 0 && (label[j], j) < (label[i], i))`; `max()` reads the other label in one step and writes its own in a second step). Give a schedule that puts both threads in the critical section.
16. `[L06]` (2 marks) The Bakery labels are never reset. What problem does this cause in a real program, and why does it rarely matter with 64-bit integers?
17. `[L08-TLP]` (4 marks) Explain, using cache-line states and invalidations, why a TTAS lock causes much less memory traffic than a TAS lock while the lock is held.
18. `[L08-TLP]` (3 marks) Compare snooping and directory-based cache coherence. Which one scales to many cores, and why?
19. `[L08-TLP]` (3 marks) Name four atomic read-modify-write operations from the lecture. Which one can be used to build the others, and how?

## C. Exam style

20. `[trace][L09]` (6 marks) A program uses this spin lock:
    ```c
    void acquire() { while (TestAndSet(&lock) == 1) {} }
    void release() { lock = 0; }
    ```
    It runs on **one core** with a strict-priority scheduler (the highest-priority ready thread always runs). Threads A and B have high priority, thread C low priority. A and B are waiting for I/O. C calls `acquire()` and enters its critical section; then the I/O finishes, and A and B become ready and call `acquire()`.
    (a) What happens next? (2) (b) Is this a deadlock or a livelock? Justify. (2) (c) Would the problem disappear on a machine with 2 cores? With 3 cores? How would you fix it on one core? (2)
    Check (a) with `tools/interleave/programs/tas_priority.il`.
21. `[trace][L08-TLP]` (4 marks) Suppose `TestAndSet` in question 20 were **not** atomic: it reads `lock` into `temp` in one step and writes `lock = 1` in a second step. Give a schedule of two threads that (a) breaks mutual exclusion, and (b) leaves `lock == 1` with no thread in the critical section. What happens after (b)?
22. `[design]` (4 marks) A server runs 32 threads on a 4-core machine. Lock X protects a counter (critical section: about 50 nanoseconds). Lock Y protects a file write (critical section: about 10 milliseconds). For each lock, would you choose a spin lock or a blocking lock? Justify both.
23. `[calc][L08-TLP]` (3 marks) 16 threads each update their own element of `long count[16]` (8 bytes per `long`; the array starts at a cache-line boundary; lines are 64 bytes). How many cache lines does the array use, and how many threads share each line? What is the fix, and how much memory does the fixed array take?

## D. Code

24. `[code]` (5 marks) Write the Bakery lock's `lock(i)` and `unlock(i)` in pseudocode from memory. Then explain in two sentences why it is first-come-first-served.
25. `[code]` (4 marks) Write `fetch_and_increment(x)` using only `read` and `CAS(x, expected, new)`. Explain why no increment is lost when many threads call it at once.
