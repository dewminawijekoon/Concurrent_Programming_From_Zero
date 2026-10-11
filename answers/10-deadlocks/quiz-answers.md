# 10 · Quiz answers: deadlocks

These answers are about the length a marker expects for the marks shown.
"Why" is always part of the answer. Every table was checked with
`calc/banker.c`.

## A. True / false

1. **False.** Unsafe means the system can no longer *guarantee* that all
   processes finish if each asks for its full maximum. Deadlock is possible
   but not certain: processes may ask for less, or release early.
2. **True.** A process often cannot know all its needs at the start (they
   depend on the input), and holding everything from the start leaves
   resources idle that others could use, which lowers concurrency. A
   process needing many resources may also wait a long time (starve) until
   all are free at once (L11 slide 21).
3. **False.** Another instance of a resource on the cycle may be held by a
   process outside the cycle, which can finish and release it, breaking the
   wait. Use the detection algorithm with the matrices instead.
4. **True.** The four conditions are all necessary. If one can never hold,
   the situation the definition describes can never arise.
5. **False.** It prevents **circular wait**. Threads still hold a lock
   while waiting for the next one (hold and wait still happens), but the
   waits can never form a cycle.
6. **False.** Memory is preemptable: the system can copy a process's pages
   to disk, give the memory to someone else, and bring the pages back later
   without harming the process (L11 slide 4).
7. **False.** It is reasonable when deadlocks are very rare and prevention
   or avoidance would be expensive or restrictive; UNIX and Windows mostly
   do this (slide 9). It trades correctness for convenience.
8. **False.** Whoever gets L1 first can also get L2: the other thread is
   waiting for L1 and holds nothing. No cycle of waiting can form (the
   explorer confirms: no deadlock).

## B. Short answer

9. **Mutual exclusion:** a mutex has one owner at a time. **Hold and
   wait:** thread 1 keeps L1 while it waits for L2. **No preemption:** no
   one can force a thread to give up a mutex; only the owner unlocks it.
   **Circular wait:** thread 1 waits for thread 2 (holder of L2), and
   thread 2 waits for thread 1 (holder of L1).
10. Free = 12 − 9 = 3. Needs: P 5, Q 2, R 7.
    (a) Q fits (2 ≤ 3): free 3 + 2 = 5. P fits (5 ≤ 5): free 10. R fits
    (7 ≤ 10): free 12. **Safe**, sequence **Q, P, R**.
    (b) Pretend: R holds 3 (needs 6), free 2. Q fits: free 4. Now P needs 5
    and R needs 6, but only 4 are free. **Unsafe, so R must wait** (even
    though a free unit exists).
11. (a) Allocation sums = (6 4 4), so **Available = (2 1 2)**.
    Need: P0 (2 2 1), P1 (2 2 1), P2 (2 0 3), P3 (0 1 1).
    (b) work (2 1 2). P3 fits → work (5 2 3). P0 fits → (6 2 5). P1 fits →
    (8 3 5). P2 fits → (8 5 6). **Safe**: P3, P0, P1, P2.
    (c) (1 0 1) ≤ Need P2 (2 0 3) and ≤ Available. Pretend: Available
    (1 1 1), P2 holds (1 2 2), needs (1 0 2). P3 fits → (4 2 2); P0 → (5 2 4);
    P1 → (7 3 4); P2 → (8 5 6). **Safe: grant.**
    (d) (1 1 0) ≤ Need P0 and ≤ Available. Pretend: Available (1 0 2).
    P0 now needs (1 1 1), P1 (2 2 1), P3 (0 1 1): all need a Y, and none is
    free; P2 needs 2 X, only 1 free. Nobody fits: **unsafe, P0 must wait.**
12. (a) C sums = (4 4), so A = (1 0). P4 asks for nothing: finishes,
    A = (1 1). P3 (1 1) fits: A = (2 2). P1 (2 0) fits: A = (3 4). P2 (0 2)
    fits: A = (5 4). **No deadlock.**
    (b) A = (1 0). P4 finishes: A = (1 1). P3 now wants (1 2): only 1 Y
    free. P1 wants 2 X (1 free), P2 wants 2 Y (1 free). Nobody fits:
    **P1, P2 and P3 are deadlocked.**
13. **Preemption:** take a resource from its owner and give it to another
    process; only possible for some resources, often by hand.
    **Rollback:** save processes' states at checkpoints; restart a
    deadlocked process from an earlier checkpoint, freeing what it got
    since; needs checkpoints and loses the work done since. **Killing:**
    kill a process in the cycle so the others get its resources; its work
    is lost, so pick one that can safely be rerun.
14. A process may only wait for a resource with a **higher** number than
    every resource it holds. Suppose there were a cycle P1 → P2 → … → P1,
    where each waits for a resource held by the next. Following the cycle,
    the number of the resource being waited for strictly increases at
    every step. Coming back to P1 we would need a number bigger than
    itself: impossible. So no cycle, so no circular wait.
15. **Same order** (both L1 then L2): deadlock-free; whoever gets L1 can
    always get L2, because the other holds nothing while waiting for L1.
    **Opposite order:** can deadlock. 1. T1 locks L1. 2. T2 locks L2.
    3. T1 waits for L2. 4. T2 waits for L1. Each holds what the other
    needs: deadlock (explorer: `m1_slide14`).

## C. Exam style

16. (a) After line 1 of each:

    | thread | Allocation (R1 R2) | Still needed (R1 R2) |
    |---|---|---|
    | T1 | 2 0 | 0 2 |
    | T2 | 0 2 | 2 0 |
    | T3 | 1 0 | 0 1 |

    Available = (3 3) − (3 2) = **(0 1)**. Safety: T3 fits (0 1) and returns
    (1 0): work (1 1). T1 needs 2 R2, T2 needs 2 R1: neither fits.
    **Unsafe** (in fact T1 and T2 can never finish from here).
    (b) 1. T1: acquire(R1, 2) → available (1 3). 2. T2: acquire(R2, 2) →
    available (1 1). Now T1 waits for 2 R2 (only 1 free) and T2 waits for
    2 R1 (only 1 free). T3 can still run to the end, but it gives back only
    what it took, so available returns to (1 1): **T1 and T2 are
    deadlocked**, each holding what the other needs.
    (c) Run them one at a time: T1 lines 1–3, then T2 lines 1–3, then T3
    lines 1–3. Each finds everything free when it starts.
    (Check: `banker schedule` with this input prints both schedules.)
17. (a) Transfer X: a → b locks a, then waits for b. At the same time
    transfer Y: b → a locks b, then waits for a. Each holds what the other
    needs: deadlock. (b) Always lock the account with the **smaller id**
    first, whatever the direction. This removes **circular wait**: every
    thread waits only for a higher id than it holds, so no cycle can form.
    (c) `transfer(a, a)` would lock the same mutex twice; a normal mutex is
    not recursive, so the thread waits for itself forever. Handle
    `from == to` separately: lock once (or do nothing). (Exercise 6.)
18. With spooling, only one printer *daemon* ever uses the printer. Other
    processes write their output to a queue on disk, which many can do at
    once, so no process ever waits holding "the printer": the printer is
    never assigned to them. A lock exists precisely to let one thread at a
    time **read and update** shared data; queueing the update elsewhere
    does not let a thread read the current value and act on it, so
    exclusive access cannot be removed (not every resource can be spooled,
    L11 slide 20).

## D. Code

19. ```
    is_safe(Available, Max, Alloc):
        Need = Max - Alloc
        work = Available;  finished[p] = false for all p;  seq = []
        repeat:
            find p with !finished[p] and Need[p] <= work   (every column)
            if none: break
            work = work + Alloc[p];  finished[p] = true;  seq.append(p)
        return (all finished), seq
    ```
20. ```
    detect(E, C, R):
        A = E - (column sums of C);  marked[p] = false for all p
        repeat:
            find p with !marked[p] and R[p] <= A   (every column)
            if none: break
            A = A + C[p];  marked[p] = true
        deadlocked = { p : !marked[p] }
    ```
