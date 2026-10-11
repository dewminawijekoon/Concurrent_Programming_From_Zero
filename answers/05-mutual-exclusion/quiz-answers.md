# 05 · Quiz answers: mutual exclusion

These answers are about the length a marker expects for the marks shown.
"Why" is always part of the answer.

## A. True / false

1. **False.** A thread may halt only in its *non-critical* section (L05
   slide 3). A thread halting inside its critical section would hold
   everyone else out forever, and no protocol can prevent that.
2. **False.** Deadlock would need P1 stuck on `turn == 2` *and* P2 stuck on
   `turn == 1` at the same time, but `turn` has one value. (It fails a
   different requirement: progress without contention.)
3. **False.** Attempt 4 is deadlock-free (some schedule always lets a thread
   in) yet it livelocks: the lock-step schedule never lets anyone in.
   Livelock is a separate liveness failure.
4. **True.** If thread i is in the CS then `victim == j`, and `victim`
   cannot equal both 0 and 1 at once (L06 slide 22). It fails because a
   thread running *alone* waits forever.
5. **False.** It uses only atomic loads and stores of `flag[]` and `victim`.
   That is the point of these algorithms (L05 slide 5).
6. **False in general, true for Peterson.** Deadlock-freedom says *some*
   thread gets in. Starvation-freedom says *every* trying thread gets in.
   Peterson happens to have both (slides 25–26), but a deadlock-free lock
   can still starve a thread (e.g. a test-and-set spin lock, module 06).
7. **False.** A CPU core does not make its writes visible to other cores
   straight away. It may already *read* another variable while its own
   write is still waiting. So each thread can write its own flag, then read
   the other's flag as still false, and both go in. `volatile` only stops
   the *compiler* from caching the variable; it does not stop the *CPU*
   from doing this. You need a memory fence, or atomic variables.
   `break_it/peterson_plain.c` shows the failure on a real machine.

## B. Short answer

8. Mutual exclusion (**safety**); no deadlock (**liveness**); no starvation
   (**liveness**); success in the absence of contention (**liveness**).
   (L03 lists absence of deadlock under safety; see `NOTES_ERRATA.md`.)
9. 1. P1: `c1 = 0`. 2. P2: `c2 = 0`. 3. P1 tests c2 (0), keeps waiting.
   4. P2 tests c1 (0), keeps waiting. Both wait forever: nobody can ever
   change c1 or c2 again.
10. 1. P1 `c1=0`. 2. P2 `c2=0`. 3. P1 sees c2==0, enters the loop. 4. P2 sees
    c1==0, enters the loop. 5. P1 `c1=1`. 6. P2 `c2=1`. 7. P1 `c1=0`.
    8. P2 `c2=0`. Back to the state after step 2: repeat forever.
    *Difference:* in a deadlock no thread can take a step. In a livelock
    threads keep taking steps (changing state) but never make progress, and
    a slightly different schedule *would* succeed.
11. Assume, to get a contradiction, that both are in their CS. Call the
    thread that entered first P1 (the other case is the same with names
    swapped): P1 entered at time t1, P2 entered later at t2, and P1 was
    still inside the whole time between t1 and t2. At t1 P1 saw `c2 == 1`, so P2 had
    not yet executed `c2 = 0`; P2 executes it after t1 and before t2. At t2
    P2 saw `c1 == 1`. But P1 set `c1 = 0` before t1 and only resets it in
    its post-protocol, which it has not reached. So `c1 == 0` at t2.
    Contradiction.
12. LockOne (`flag`) contributes "**announce interest**": it never blocks a
    thread running alone, but deadlocks when both run concurrently (both
    raise, both wait). LockTwo (`victim`) contributes "**let the other go
    first**": it is fine when both run concurrently (the last writer of
    victim waits), but deadlocks a thread running alone. Peterson waits
    only while *both* `flag[j]` and `victim == i` hold, so neither failure
    case blocks.
13. 1. T0: `victim = 0`. 2. T1: `victim = 1`. 3. T1: `flag[1] = true`.
    4. T1 tests: `flag[0]` is false, so it enters the CS.
    5. T0: `flag[0] = true`. 6. T0 tests: `victim == 0`? No, it is 1, so it
    enters the CS too. Writing `flag` first guarantees that whoever writes
    `victim` last will see the other's flag.
14. Thread i waits only while `flag[j] && victim == i`. If j is in its CS, it
    eventually leaves and sets `flag[j] = false`, letting i in. If j instead
    leaves and immediately tries again, it executes `victim = j` on the way
    in, which makes i's condition false. So j can enter at most once while
    i waits: i cannot be overtaken forever.

## C. Is it correct?

15. **(a) No mutual exclusion:** 1. T0 sees busy==0. 2. T1 sees busy==0.
    3. T0 sets busy=1, enters. 4. T1 sets busy=1, enters. The test and the
    set are separate steps, the same flaw as Attempt 2. **(b) No deadlock:**
    a thread waits only while busy==1, and busy==1 means some thread has
    entered or is in the CS and will reset it. The explorer agrees:
    `mutex=violated deadlock=none`. It also finds starvation. *This is
    exactly why hardware provides an atomic test-and-set (module 06).*
16. A thread that backs off sets its flag to 1 and **never raises it
    again**, so it can fall out of the loop and enter its CS with its flag
    down. Schedule (from the explorer):
    1. P1 is in the CS (c1 = 0).
    2. P2: `c2 = 0`, sees `c1 == 0`, enters the loop.
    3. P1 leaves: `c1 = 1`, `turn = 2`.
    4. P2 (it saw turn == 1 earlier) backs off: `c2 = 1`.
    5. P2 re-tests `c1 == 0`: false, so it leaves the loop and enters the CS
       with `c2 == 1`.
    6. P1 comes back: `c1 = 0`, sees `c2 == 1`, enters.

    **Both are in the CS.** Dekker avoids this by waiting for its turn with
    the flag down and then *raising it again* (`c1 = 0`) before re-testing.
17. (1) They show what mutual exclusion *needs*, so you can reason about any
    synchronization code, including your own (exam and interview staple).
    (2) Real locks are built on the same ideas, plus hardware atomics: flags
    to announce, tie-breaking for fairness, spinning vs blocking. (3) They show
    that real CPUs can reorder memory operations, which matters whenever
    you write code that shares variables without a lock.

## D. Code

18. ```
    lock(i):   flag[i] = true;  victim = i;
               while (flag[j] && victim == i) {}      // j = 1 - i
    unlock(i): flag[i] = false;
    ```
    It guarantees mutual exclusion, deadlock-freedom and starvation-freedom
    (and works without contention).
19. The tree has n/2 nodes at the bottom level, n/4 above them, …, 1 at the
    root: n/2 + n/4 + … + 1 = **n − 1** Peterson locks. Each thread acquires one lock
    per level: **log₂ n** locks per `lock()`. For n = 4 that is 3 locks,
    2 per thread.
