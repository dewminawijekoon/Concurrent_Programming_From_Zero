# 06 · Quiz answers: n-thread locks and the hardware

These answers are about the length a marker expects for the marks shown.
"Why" is always part of the answer.

## A. True / false

1. **False.** It uses **n − 1** levels (L06 slide 27). Each level stops at
   least one thread when several try, so n − 1 levels are enough to go from
   n threads down to 1.
2. **False.** A thread can be overtaken by one that arrived later. Example:
   P0 writes `level[0] = 1` first, but P1 writes `victim[1]` before P0
   does, so P0 is the victim of level 1 and P1 enters first (slide 33:
   "threads can be overtaken"; `demo/filter_overtake.il`).
3. **False.** The doorway reads the labels one at a time. Two threads can
   read them at the same moment, see the same maximum and take the same
   label. That is exactly why the pair (label, id) is compared: the pairs
   are never equal.
4. **False.** The lock stays correct; the explorer checks it with
   non-atomic `max()` for 2 and 3 threads. A non-atomic `max()` only causes
   equal labels, and the id breaks the tie. The flag, raised before reading
   the labels, makes it safe.
5. **False.** Starvation-free only says every thread gets in
   *eventually*, possibly after being overtaken many times. The Filter lock
   is starvation-free but not first-come-first-served. (The other direction
   is true: first-come-first-served implies starvation-free.)
6. **False.** Both give the same guarantees: mutual exclusion and no
   deadlock, but not starvation-freedom. TTAS is only **faster**: waiters
   spin on their own cached copy instead of writing the lock again and again.
7. **False.** That is *true* sharing. False sharing is when threads write
   **different** variables that lie in the **same cache line**; the line
   bounces between caches although no data is shared (L08-TLP slide 22).
8. **False.** On a multicore machine with very short critical sections,
   spinning is often the fastest choice: the waiter notices the release at
   once, while going to sleep and waking up costs much more. Spinning is bad
   when waits are long, when there are more threads than cores, or on one
   core with priorities (L09 slide 3).
9. **False.** Nothing records who has waited longest. One thread can win
   the test-and-set every time while another never does. The explorer finds
   this run for `programs/tas_lock.il`.
10. **False.** Write-update sends every new value to every other cache, so
    it needs **more** bandwidth (L08-TLP slide 14). Write-invalidate sends
    one invalidation, after which the writer can write many times locally.

## B. Short answer

11. The **doorway** is the first part of `lock()`, which a thread always
    finishes in a bounded number of its own steps, without waiting (Bakery:
    raise the flag and take a label). **Bounded waiting** (r-bounded): after
    A finishes its doorway, other threads can enter ahead of A at most r
    times. **First-come-first-served**: if A finishes its doorway before B
    starts its doorway, A enters before B (r = 0). "Starvation-free" only
    says a thread gets in eventually; it puts no limit on how long it waits
    or how often it is overtaken (slide 34).
12. (Checked with `demo/filter_overtake.il --why "0 1 CS 1 CS 0"`.)
    1. T0: `level[0] = 1`.
    2. T1: `level[1] = 1`.
    3. T1: `victim[1] = 1`.
    4. T0: `victim[1] = 0`.
    5. T1 checks level 1: `victim[1]` is 0, not 1, so T1 climbs: `level[1] = 2`, `victim[2] = 1`.
    6. T1 checks level 2: nobody else is at level ≥ 2, so **T1 enters**.
    7. T0 checks level 1: T1 is at level 2 ≥ 1 and `victim[1] == 0`, so T0 waits until T1 leaves.
13. Suppose n − L + 1 threads got past level L. Take the one that wrote
    `victim[L]` last, X. Every other one wrote `level = L` before writing
    `victim[L]`, so before X's write. When X checked, it saw them at level
    ≥ L and saw `victim[L] == X`, so it had to wait: it cannot have got past.
    Contradiction. For L = n − 1: at most one thread gets past the last
    level, so at most one is in the critical section: **mutual exclusion**.
14. Two threads can take the same label (non-atomic `max()`), so labels
    alone cannot always pick a winner; the id is a tie-breaker that is
    different for every thread. With `label[k] < label[i]`: both threads
    have label 1, neither label is smaller, so neither waits and **both
    enter** (mutual exclusion broken). With `<=`: each sees the other's
    equal label and waits for it: **deadlock**. (Both checked with
    `demo/bakery_ties.il`.)
15. (Checked with model exercise m3.)
    1. P0 reads `label[1] = 0` (it will write 1 next).
    2. P1 reads `label[0] = 0`.
    3. P1 writes `label[1] = 1`.
    4. P1 checks: `label[0] == 0`, "P0 is not interested", so **P1 enters**.
    5. P0 writes `label[0] = 1`.
    6. P0 checks: is (1, 1) < (1, 0)? No, so **P0 enters too**.

    P1 looked at P0 while P0 was half-way through taking its label. With the
    flag, P0 would have raised `flag[0]` before step 1, and P1 would have
    waited for P0's label (and then lost the tie to the smaller id).
16. A fixed-size integer eventually **overflows** and wraps around to a
    small (or negative) number. A thread with a wrapped label would look
    like it arrived first, breaking first-come-first-served and possibly
    mutual exclusion. A 64-bit label would need about 10¹⁹ lock calls to
    overflow, which takes centuries even at a billion calls per second.
17. While A holds the lock, each TAS waiter executes test-and-set again and
    again. Each one is a **write**, so the waiter's cache must get the line
    in exclusive state and **invalidate** every other copy. The next waiter
    does the same, so the line moves between caches all the time, flooding
    the bus, and the holder must fight for the line to unlock. A TTAS waiter
    only **reads** while the lock looks taken: after the first miss it has a
    **shared** copy and every further read hits its own cache, with no bus
    traffic. Traffic happens only when the holder writes `false` (one
    invalidation), after which the waiters re-read and try one
    test-and-set each.
18. **Snooping:** every cache watches a shared bus and keeps its own record
    of the state of its lines; every miss or invalidation is broadcast to
    all. Simple and fast for a few cores. **Directory:** one place (possibly
    split across memory modules) records which caches hold each line, and
    sends messages only to those caches. The **directory** scales: snooping
    needs every request broadcast to everyone, so the shared bus becomes the
    bottleneck as cores are added (L08-TLP slides 12, 20, 24).
19. Atomic exchange, test-and-set, fetch-and-increment, compare-and-swap
    (also load-linked / store-conditional). **Compare-and-swap** can build
    the others with a retry loop: read the old value, compute the new value,
    `CAS(x, old, new)`; if it fails, someone changed x, so read again and
    retry. (LL/SC can be used the same way.)

## C. Exam style

20. (a) C holds the lock. When A and B become ready, they have higher
    priority, so the scheduler stops C and runs A and B. Each calls
    `TestAndSet`, gets 1, and loops. A spinning thread is **running**, so it
    is always ready, and A and B always outrank C. C never runs again, so
    it never calls `release()`, and the lock stays 1 forever. A and B spin
    forever. (Explorer, `tas_priority.il --priority A=2,B=2,C=1 --no-halt`:
    LIVELOCK, A and B keep trying.)
    (b) A **livelock**: A and B keep executing instructions (they are not
    blocked), but no thread ever makes progress. In a deadlock, no thread
    could take a step at all.
    (c) **2 cores: no.** With strict priorities, the two cores run the two
    highest-priority ready threads, A and B, both spinning; C still never
    runs. **3 cores: yes**, C gets the third core and releases the lock.
    Fix on one core: make waiting threads **block** (sleep and wakeup, a
    semaphore or mutex), so a waiting A or B is not ready and C can run
    (model exercise m2). [EXTRA] Or raise C's priority while it holds the
    lock (priority inheritance), or have the spinner yield/sleep.
21. (a) 1. T0: `temp = lock` (0). 2. T1: `temp = lock` (0). 3. T0:
    `lock = 1`; temp was 0, so T0 enters. 4. T1: `lock = 1`; temp was 0, so
    T1 enters. Both are inside.
    (b) 1. T0 takes the lock normally (reads 0, writes 1) and enters.
    2. T1: `temp = lock` (1). 3. T0 leaves: `lock = 0`. 4. T1: `lock = 1`;
    temp was 1, so T1 loops. Now `lock == 1` and nobody is inside.
    **After (b):** every thread that tries reads 1 and writes 1 again. Only
    a thread leaving the critical section ever writes 0, and nobody is in
    it. All threads spin forever: a livelock. (Explorer:
    `programs/tas_nonatomic.il` finds both; `make run M=06 EX=tas_nonatomic`
    shows both on real hardware.)
22. **Lock X (50 ns): spin**, or better spin-then-block. The wait is far
    shorter than the cost of putting a thread to sleep and waking it
    (microseconds), so a waiter that spins briefly gets the lock sooner. But
    with 32 threads on 4 cores the holder can be descheduled while holding
    the lock, and then spinners waste whole time slices; so spin only for a
    short while, then block (this is what `pthread_mutex_t` does).
    **Lock Y (10 ms): block.** Spinning for 10 ms wastes a core that 28
    other runnable threads could use; with more threads than cores it may
    even keep the holder off the CPU. A sleeping waiter uses no CPU.
23. 16 × 8 = 128 bytes = **2 cache lines**, with **8 threads per line**.
    Every update by one thread invalidates the line in the other 7 threads'
    caches (false sharing). **Fix:** pad each counter to its own line, for
    example `struct { _Alignas(64) long v; } count[16];`. That takes
    16 × 64 = **1024 bytes** (8 times more memory, but no false sharing).

## D. Code

24. ```
    lock(i):
        flag[i] = true
        label[i] = max(label[0], ..., label[n-1]) + 1
        while (exists k != i: flag[k] and (label[k], k) < (label[i], i)) {}
    unlock(i):
        flag[i] = false
    ```
    If A finishes its doorway before B starts its doorway, B's `max()` reads
    A's finished label, so B's label is bigger. B then waits while A's flag
    is up, so A enters first.
25. ```
    fetch_and_increment(x):
        loop:
            old = read(x)
            if CAS(x, old, old + 1): return old
    ```
    The CAS writes `old + 1` only if x still equals the value we read. If
    another thread incremented x in between, x is no longer `old`, the CAS
    fails and changes nothing, and we retry with the fresh value. So every
    successful CAS adds exactly 1 to the latest value, and each old value is
    returned to exactly one thread (exercise 5 checks this).
