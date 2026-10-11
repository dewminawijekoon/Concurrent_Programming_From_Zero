# 01 · Quiz answers: races and interleavings

These answers are about the length a marker expects for the marks shown.
"Why" is always part of the answer. Every output and trace answer was
checked with `tools/interleave`; the model files are in
[`puzzles/`](puzzles) next to this file, and the commands are given with
each answer.

## A. True / false

1. **False.** It is a load, an add and a store (L03 slides 37–39). Another
   thread can load or store between them. Even a single `inc` instruction
   is not atomic across cores unless the processor locks memory for it
   (in C: `atomic_fetch_add`).
2. **False.** Each thread stores "the value it loaded plus one", and the
   loaded value is at least 0. So the last store writes at least 1. The
   possible results are 1 and 2.
3. **False.** A store always writes "a value that was in memory earlier,
   plus one", so updates can be lost but never invented. The result is at
   most N × M.
4. **False.** A race may need one rare interleaving, and there are far too
   many interleavings to test them all (L03 slides 32, 34). Testing can
   show a race exists, never that none exists. ThreadSanitizer or a proof
   is needed.
5. **True.** (2·3)! / (3!)² = 720 / 36 = 20. Equivalently, choose which 3 of
   the 6 time slots belong to the first process: C(6, 3) = 20.
6. **False.** If P2 gets the processor first, it spins in `while (continue)`
   forever. It never finishes and never blocks, so the scheduler never
   takes the processor away and P1 never runs. P1 is ready all the time
   but is ignored forever, which is exactly what "unfair" means (L03
   slides 32, 36).
7. **False.** Bigger numbers take longer to test (more trial divisions), so
   the later blocks are more work, and the threads finish at different
   times (L01 slide 19). Module 00 exercise 2 measures this: the last of 4
   blocks needs more than twice the divisions of the first.
8. **True.** "The counter is never wrong" is a "nothing bad ever happens"
   property. One finite interleaving that ends with a wrong value is enough
   to break it, which is the mark of a safety property.
9. **False.** The answer becomes correct, but the program gets **slower**.
   Every increment now pays for a lock and an unlock, and the threads queue
   for the one lock (lock contention). Our measurement: 16 threads were
   about 440 times slower than the local-sum version.

## B. Traces, outputs and counting

10. Each `counter++` is: load counter into a register, add 1 to the
    register, store the register into counter. Timeline (counter starts at 5):

    | step | thread 1 | thread 2 | counter |
    |--:|---|---|:--:|
    | 1 | load (R1 = 5) | | 5 |
    | 2 | | load (R2 = 5) | 5 |
    | 3 | R1 = 6 | | 5 |
    | 4 | | R2 = 6 | 5 |
    | 5 | store R1 | | 6 |
    | 6 | | store R2 | **6** |

    Two increments ran, but the counter went up by only 1: thread 2's store
    overwrote thread 1's update with a value computed from the old 5.

11. (a) (3·2)! / (2!)³ = 720 / 8 = **90**. (b) (2·4)! / (4!)² = 40320 / 576 =
    **70**. (c) (n·m)! counts every order of the n·m steps as if any step
    could go anywhere. But each process's own m steps must stay in program
    order. Of the m! ways to order one process's steps, only one is
    allowed, so we divide by m! once per process: (m!)ⁿ.

12. P1 is really `a = y; b = z; x = a + b`. Possible values: **0, 1, 2, 3**.
    - 0: P1 does all its steps first.
    - 1: P2 `y = 1`; P1 loads y (1) and z (0); P2 `z = 2`.
    - 2: P1 loads y (0); P2 `y = 1`, `z = 2`; P1 loads z (2).
    - 3: P2 finishes first; P1 loads 1 and 2.

    If z is loaded first, **2 becomes impossible**: if P1 sees z = 2, then
    P2 has already done y = 1 (it comes first in P2), so P1 also sees y = 1
    and gets 3. Only 0, 1, 3 remain.
    Check: `interleave.py exercises/models/m2_x_equals_y_plus_z.il --outputs`
    (in this answers folder).

13. (a) **9** (P1 then P2: 2·3 = 6, then 6 + 3) or **15** (P2 then P1:
    2 + 3 = 5, then 5·3). (b) **5, 6, 9 and 15.** The new values come from a
    lost update: both load c1 = 2 before either stores.
    - 6: P2 loads 2, P1 loads 2, P2 stores 5, P1 stores 2·3 = 6.
    - 5: P1 loads 2, P2 loads 2, P1 stores 6, P2 stores 2 + 3 = 5.

    Check: `interleave.py puzzles/p3_mul_add.il --outputs` gives 5, 6, 9, 15.

14. **`P Q` is impossible.** Suppose both print. P printed, so when P read
    b it was 0: P's read of b came before Q's write `b = 1`. Q printed, so
    Q's read of a came before P's write `a = 1`. But each thread writes
    before it reads. So: P writes a → P reads b → Q writes b → Q reads a.
    Then Q reads a **after** P wrote 1, and Q cannot print. Contradiction.
    **Printing nothing is possible:** P `a = 1`, Q `b = 1`, then both tests
    fail. Check: `interleave.py puzzles/p1_flags.il --outputs` gives
    (nothing), `P`, `Q`. (This is the idea behind the "flag principle" in
    L01 slide 46. On real hardware without fences, `P Q` *can* happen; see
    module 05, section 6.)

15. (a) **No.** A prints its second A only if x == 2. Only B sets x = 2,
    and then B prints **two** B's (one inside the `if`, one after). So a
    second A always comes with two B's: `A A B B` is possible, `A A B` is not.
    (b) **Yes:** A `x = 1`; B sees x == 1, sets x = 2, prints B; A prints A;
    A sees x == 2, prints A; B prints its last B.
    (c) **No.** Two B's mean B set x = 2, which needs A's `x = 1` first. Then
    when A tests x it is 2 (nobody changes it back), so A prints A twice:
    `B B A A`, never `B B A`.
    (d) `B A`, `B A A B`, `B A B A`, `B B A A`.
    Check: `interleave.py puzzles/p4_letters.il --outputs` and `--why "A A B"`.

16. (a) **Yes.** A: `x = 1`, print A; B sees 1, sets 0, prints B; repeat.
    (b) **No.** After A's first `x = 1`, x stays 1 until B resets it. With a
    fair scheduler, B eventually runs its test while x == 1 and prints B.
    So B cannot be silent forever. (c) **Yes.** A: `x = 1`; B tests x
    (true); A prints A, sets x = 1 again, prints A; B sets x = 0, prints B;
    repeat. Each round prints A A B, and B runs every round, so this is fair.
    Check: `interleave.py puzzles/p5_forever.il --infinite "A B"` (possible),
    `--infinite "A"` (not possible), `--infinite "A A B"` (possible).

17. Possible values: **2, 3, 4**. The smallest, 2:
    1. T0 loads 0, stores 1 (first increment done).
    2. T0 loads 1 (start of its second increment).
    3. T1 loads 1, stores 2; T1 loads 2, stores 3 (T1 is finished).
    4. T0 stores 1 + 1 = **2**, wiping out both of T1's increments.

    It cannot be 1: T0's last store writes "something it loaded, plus one",
    and its second load happens after its own first store, so it loads at
    least 1 and stores at least 2. The same holds for T1.
    Check: `interleave.py puzzles/p2_two_increments.il --outputs` and `--why 2`.

## C. Lab-style analysis

18. Usually **yes, up to the number of cores.** Updates are lost only when
    two threads are inside their load-add-store at the same moment. More
    threads running at once means more overlaps, so the percentage lost
    grows (we measured 40%, 61%, 71%, 80% for N = 2, 4, 8, 16 on 16 cores).
    On a 4-core machine, expect the loss to grow from N = 2 to N = 4. After
    that, only 4 threads can really run at once. Extra threads mostly take
    turns, so the loss grows more slowly or levels off, and it varies a lot
    between runs. Each thread switch in the middle of an increment can
    still lose updates.
19. In Part A each of the N × M increments takes and releases the lock:
    **800,000** lock operations for N = 8, M = 100,000. Only one thread can
    hold the lock, so the others queue up and wait. This waiting is **lock
    contention**. The lock's memory also has to move between cores each
    time a different thread takes it, which is slow. In Part B each thread
    counts privately with no waiting and locks only once: **8** lock
    operations in total. Both give the correct N × M, but B removes almost
    all the waiting.
20. The lock stays held forever. The next thread that calls `lock()` waits
    forever, and so does the same thread if it tries to lock again (a
    normal mutex does not let its owner lock it twice). The program hangs:
    a deadlock. `make run M=01 EX=lab2_mutex_vs_local ARGS=forget` shows it.
21. (a) Every number needs one trip to the shared counter, protected by a
    lock (or an atomic instruction). Testing a small number is very fast,
    so the threads spend much of their time queueing for the counter
    (contention), not testing. (b) Hand out **batches**: each call takes,
    say, 1000 numbers (`start = get_and_add(&counter, 1000)`), and the
    thread tests `start .. start + 999`. The counter is touched 1000 times
    less often, and the load is still balanced because batches are small
    compared with the whole range.
22. **Safety:** something bad never happens; disproved by one finite run.
    **Liveness:** something good eventually happens; disproved by an
    infinite run where it never does. (a) safety; (b) liveness (no
    starvation); (c) L01 slide 34 calls it **liveness**, L03 slide 43 lists
    it under **safety**. Most books say liveness ("some thread eventually
    gets in"). Say which definition you use.

## D. Code and proof

23. The original `temp = value; value = temp + 1;` can interleave so that
    two threads both read the same `value` before either writes.
    ```c
    // (a) with a mutex
    long get_and_increment(struct counter *c) {
        pthread_mutex_lock(&c->lock);
        long temp = c->value;
        c->value = temp + 1;
        pthread_mutex_unlock(&c->lock);
        return temp;
    }
    // (b) with a C11 atomic: atomic_long value;
    long get_and_increment(atomic_long *value) {
        return atomic_fetch_add(value, 1);   // read and add as one step; returns the old value
    }
    ```
24. While a thread holds the mutex, no other thread can be between its own
    load and store of the counter. So no two increments overlap: they
    happen one after another in some order. Each increment loads the value
    stored by the previous one and stores one more. Starting from 0, after
    N × M increments the counter is N × M.
25. The check (`seats_left > 0`) and the action (`seats_left--`) are in two
    **separate** critical sections. Two threads can both see 1 seat left,
    both release the lock, and both book it: `seats_left` becomes −1. This
    is check-then-act. Fix: one critical section around both.
    ```c
    int book_seat(void) {
        pthread_mutex_lock(&m);
        int ok = seats_left > 0;
        if (ok) seats_left--;
        pthread_mutex_unlock(&m);
        return ok;
    }
    ```
