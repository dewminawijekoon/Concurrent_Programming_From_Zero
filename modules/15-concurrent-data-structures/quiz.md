# 15 · Quiz: concurrent data structures

Answer on paper first, as you would in the exam, then check
[`answers/15-concurrent-data-structures/quiz-answers.md`](../../answers/15-concurrent-data-structures/quiz-answers.md).
Marks in brackets show how much detail is expected. Tags: `[L12]` lecture,
`[Lab05]` lab, `[tf]` true/false + justification, `[trace]` give an order of
steps, `[output]` which results are possible, `[calc]`, `[design]`, `[code]`.

## A. True or false? Justify briefly. (2 marks each)

1. `[tf][L12]` A lock-free method cannot contain a loop.
2. `[tf][L12]` Every wait-free method is also lock-free.
3. `[tf][Lab05]` A counter protected by a test-and-set spin lock is lock-free, because it uses no mutex.
4. `[tf][Lab05]` A list with one lock per node (hand-over-hand) always has higher throughput than a list with one global lock.
5. `[tf][L12]` In the Michael–Scott queue, if the CAS that moves `tail` fails, the enqueue has failed and must start again.
6. `[tf][L12]` In the two-lock bounded queue, an enqueue and a dequeue can run at the same time.
7. `[tf][Lab05]` The CAS-loop counter can lose an increment when many threads use it.

## B. Short answer

8. `[Lab05]` (6 marks) Classify each `increment()` as wait-free, lock-free, obstruction-free or blocking, with one sentence of justification each. Then answer: which would you choose for a hard real-time system where every operation must finish within a known time, and why?
   ```c
   (a) pthread_mutex_lock(&m); count++; pthread_mutex_unlock(&m);
   (b) long o; do { o = LOAD(count); } while (!CAS(count, o, o + 1));
   (c) FETCH_ADD(count, 1);
   (d) long o = LOAD(count); if (!CAS(count, o, o + 1)) return;   // gives up
   ```

9. `[trace][Lab05]` (4 marks) In the hand-over-hand list, a student's walk unlocks `prev` *before* locking `curr`. Give an order of steps of two threads, insert(20) and delete(12), on the list `head → 5 → 12 → 30`, that loses the insert.

10. `[Lab05]` (4 marks) Explain why the CAS-loop counter is lock-free but not wait-free. Could one thread starve?

11. `[Lab05]` (3 marks) Under heavy contention, why is `atomic_fetch_add` faster than a CAS loop?

12. `[trace][Lab05][L12]` (5 marks) A bounded queue wakes sleeping dequeuers only when an enqueue makes the queue go from empty to non-empty, and it uses `pthread_cond_signal`. Give a run with two consumers and two producers in which a consumer sleeps forever although an item is in the queue. How does `pthread_cond_broadcast` fix it?

13. `[Lab05][L12]` (4 marks) In the two-lock queue, why is the shared `permits` counter a bottleneck, and how does splitting it into `enqSidePermits` and `deqSidePermits` help? What new danger appears?

14. `[calc][L12]` (4 marks) A combining tree serves 16 threads. (a) How many levels does a thread climb? (b) If all 16 threads increment at the same moment and combine perfectly, how many times is the counter at the root updated? (c) Give one advantage and two disadvantages compared with a CAS counter.

15. `[trace][L12]` (5 marks) In the Michael–Scott queue, thread A has done its first CAS (`last->next = node`) and is then paused before moving `tail`. Thread B now calls `enq`. Describe step by step what B does. Why is this called "helping", and why does it make the queue lock-free rather than blocking?

16. `[L12]` (3 marks) Why does the queue use a sentinel node, and why does a dequeue turn the first real node into the new sentinel instead of unlinking it?

17. `[design][L12]` (4 marks) In the two-lock queue, an enqueuer that made the queue non-empty first takes `deqLock` and only then broadcasts `notEmpty`. Why must it take that lock? Why is there no risk of deadlock?

## C. Is it correct? (exam style)

18. `[output]` (4 marks) `count` starts at 0. Two threads each call the "give up" increment (8d) once, then a third thread prints `count` after both have finished. Which outputs are possible? Which are possible with the CAS loop (8b)? Check both with the explorer (`exercises/models/m1_cas_counter.il`).

19. `[EXTRA]` (4 marks) Is this history of a FIFO queue linearizable? (Time runs left to right; bars show when each call started and returned. The queue starts empty.)
    ```
    A: |-- enq(1) --|
    B:                 |-- enq(2) --|
    C:      |----------------- deq() -> 2 -----------------|
    ```
    What if `enq(1)` and `enq(2)` overlapped in time instead?

20. `[EXTRA]` (4 marks) Explain the ABA problem with a lock-free stack pop, as an order of steps. Why do the exercises in this module never free dequeued nodes?

## D. Code

21. `[code]` (3 marks) Write `get_and_increment()` with a CAS loop (L12 slide 16).

22. `[code]` (6 marks) Write the Michael–Scott `enq()` in pseudocode from memory, and mark the logical and the physical enqueue.

23. `[code]` (5 marks) Write `enq()` of the two-lock bounded queue (with `permits`, `notFull` and `notEmpty`) in pseudocode.
