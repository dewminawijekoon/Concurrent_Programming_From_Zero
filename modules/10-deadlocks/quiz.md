# 10 · Quiz: deadlocks

Answer on paper first, as you would in the exam, then check
[`answers/10-deadlocks/quiz-answers.md`](../../answers/10-deadlocks/quiz-answers.md).
Marks in brackets show how much detail is expected. Tags: `[L11]` lecture,
`[tf]` true/false + justification, `[trace]` give a schedule, `[proof]`,
`[calc]`, `[code]`, `[design]`. Check every table with
`make run M=10 EX=banker ARGS="..."`.

## A. True or false? Justify briefly. (2 marks each)

1. `[tf][L11]` An unsafe state is a deadlocked state.
2. `[tf][L11]` Preventing hold-and-wait (every process requests all its resources at the start) is not always suitable.
3. `[tf][L11]` If a resource type has several instances, a cycle in the resource-allocation graph proves that there is a deadlock.
4. `[tf][L11]` If a system makes sure that just one of the four deadlock conditions can never hold, deadlock is impossible.
5. `[tf][L11]` Numbering the locks and always taking them in increasing order prevents the hold-and-wait condition.
6. `[tf][L11]` Main memory is a non-preemptable resource.
7. `[tf][L11]` Ignoring deadlocks (the ostrich algorithm) is never a reasonable choice.
8. `[tf][L06]` Two threads that both run `L1.lock(); L2.lock(); ...; L2.unlock(); L1.unlock();` can deadlock with each other.

## B. Short answer

9. `[L11]` (4 marks) State the four conditions for deadlock. For each, give one sentence showing how it holds when two threads lock two mutexes in opposite orders.
10. `[calc][L11]` (5 marks) One resource type with 12 units. P holds 5 (maximum 10), Q holds 2 (maximum 4), R holds 2 (maximum 9). (a) Is the state safe? Give a safe sequence if there is one. (b) R asks for 1 more unit. Should the Banker grant it? Show your working.
11. `[calc][L11]` (6 marks) Resources X, Y, Z with E = (8 5 6).

    | process | Allocation | Max |
    |---|---|---|
    | P0 | 1 0 2 | 3 2 3 |
    | P1 | 2 1 0 | 4 3 1 |
    | P2 | 0 2 1 | 2 2 4 |
    | P3 | 3 1 1 | 3 2 2 |

    (a) Compute Available and the Need matrix. (b) Is the state safe? Show each step. (c) Should P2's request (1 0 1) be granted? (d) Should P0's request (1 1 0) be granted?
12. `[calc][L11]` (5 marks) Resources X, Y with E = (5 4).

    | process | C (holds) | R (requests) |
    |---|---|---|
    | P1 | 1 2 | 2 0 |
    | P2 | 2 0 | 0 2 |
    | P3 | 1 1 | 1 1 |
    | P4 | 0 1 | 0 0 |

    (a) Run the detection algorithm. Is anyone deadlocked? (b) Repeat with P3's request changed to (1 2).
13. `[L11]` (3 marks) Describe the three ways to recover from a deadlock, with one drawback each.
14. `[proof][L11]` (3 marks) Explain why requesting resources only in increasing number order makes circular wait impossible.
15. `[trace][L06]` (3 marks) L06 slide 14: "Will this combination of 2 locks be deadlock free?" Answer for both cases (same order, opposite order), with a schedule where needed.

## C. Exam style

16. `[calc][trace][L11]` (6 marks) R1 has 3 instances and R2 has 3. An `acquire` waits until all the instances it asks for are free, then takes them at once.
    ```
    T1:  1) acquire(R1, 2)   2) acquire(R2, 2)   3) release all
    T2:  1) acquire(R2, 2)   2) acquire(R1, 2)   3) release all
    T3:  1) acquire(R1, 1)   2) acquire(R2, 1)   3) release all
    ```
    (a) Each thread has finished its line 1. Write the allocation matrix, the still-needed matrix and the available vector. Is this state safe? (2)
    (b) Give a schedule from the start that ends in deadlock, and say who waits for what. (2)
    (c) Give a schedule from the start in which all threads complete. (2)
17. `[design]` (4 marks) A bank locks each account separately. `transfer(from, to, amount)` locks `from`, then `to`. (a) Show how two transfers can deadlock. (b) Fix it, and say which deadlock condition your fix removes. (c) What goes wrong if someone calls `transfer(a, a, 10)` with your fix, and how do you handle it?
18. `[design]` (3 marks) Explain how spooling removes the mutual-exclusion condition for a printer. Why can't the same trick be used for a lock that protects a shared counter?

## D. Code

19. `[code]` (5 marks) Write the Banker's safety algorithm in pseudocode from memory, returning a safe sequence.
20. `[code]` (3 marks) Write the deadlock detection algorithm (with E, A, C, R) in pseudocode.
