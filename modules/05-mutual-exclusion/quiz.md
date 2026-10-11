# 05 · Quiz: mutual exclusion

Answer on paper first, as you would in the exam, then check
[`answers/05-mutual-exclusion/quiz-answers.md`](../../answers/05-mutual-exclusion/quiz-answers.md).
Marks in brackets show how much detail is expected. Tags: `[L05]` lecture,
`[tf]` true/false + justification, `[trace]` give a schedule, `[proof]`,
`[code]`, `[design]`.

## A. True or false? Justify briefly. (2 marks each)

1. `[tf][L05]` A solution to the mutual exclusion problem may let a thread halt inside its critical section.
2. `[tf][L05]` Attempt 1 (single `turn` variable) can deadlock.
3. `[tf][L05]` Livelock is impossible if the algorithm is deadlock-free.
4. `[tf][L06]` LockTwo satisfies mutual exclusion.
5. `[tf][L06]` Peterson's algorithm needs an atomic test-and-set instruction.
6. `[tf][L06]` If Peterson's lock is deadlock-free, it is also starvation-free.
7. `[tf][EXTRA]` Peterson's lock written with plain `volatile int` in C is guaranteed correct on an x86 multicore.

## B. Short answer

8. `[L05]` (3 marks) List the four requirements a mutual exclusion solution must satisfy (L05 slides 3–4), and classify each as *safety* or *liveness*.
9. `[trace][L05]` (3 marks) Give an interleaving that shows Attempt 3 deadlocks. Number the steps.
10. `[trace][L05]` (4 marks) Give an interleaving in which Attempt 4 *livelocks*, and explain how livelock differs from deadlock.
11. `[proof][L05]` (4 marks) Prove that Attempt 3 satisfies mutual exclusion.
12. `[L06]` (3 marks) Peterson's lock is often described as "LockOne + LockTwo". What does each contribute, and in which situation does each one fail on its own?
13. `[trace][L06]` (4 marks) In Peterson's `lock()`, swap the lines `flag[i] = true;` and `victim = i;`. Show a schedule that violates mutual exclusion.
14. `[proof][L06]` (4 marks) Explain why Peterson's lock is starvation-free.

## C. Is it correct? (exam style)

15. `[trace]` (6 marks) Consider this lock for threads 0 and 1:
    ```c
    int busy = 0;
    void lock()   { while (busy == 1) {}  busy = 1; }
    void unlock() { busy = 0; }
    ```
    (a) Does it satisfy mutual exclusion? (b) Can it deadlock? Justify each with a schedule or an argument.
    Then check your answer: write the model in `tools/interleave` syntax and run it.

16. `[trace]` (6 marks) A student "fixes" Attempt 3 like this:
    ```c
    P1:  c1 = 0;
         while (c2 == 0) { if (turn == 2) c1 = 1; }   // back off forever if it's not my turn
         CS;  c1 = 1;  turn = 2;
    ```
    (P2 symmetric with `turn == 1`.) Find a problem. Hint: what happens after P1 backs off and P2 leaves the critical section?

17. `[design]` (4 marks) Why do lock algorithms like Peterson and Dekker matter today, if every OS gives you `pthread_mutex_t`? Give two reasons.

## D. Code

18. `[code]` (5 marks) Write Peterson's `lock(i)` and `unlock(i)` in pseudocode from memory, then state the three properties it guarantees.
19. `[code]` (4 marks) The tournament lock in ex4 uses three Peterson locks for four threads. How many Peterson locks does a tournament lock for *n* threads (n a power of 2) need, and how many does each thread acquire per `lock()`?
