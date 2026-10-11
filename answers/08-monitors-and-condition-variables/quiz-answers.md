# 08 · Quiz answers: monitors and condition variables

These answers are about the length a marker expects for the marks shown.
"Why" is always part of the answer.

## A. True / false

1. **True** (for a real monitor, as in L09 slide 11). Only one thread can be
   active inside a monitor at a time; the language does the locking.
   *Careful:* in C with Pthreads there is no real monitor. There **you**
   must lock the mutex at the start and unlock it at the end of every
   operation.
2. **False.** A signal with no waiting thread does nothing and is not
   remembered (L09 slide 10: "if the queue is non-empty..."). The later
   `wait(c)` sleeps until a *new* signal. This is the difference from a
   semaphore's `up()`, which is stored in the counter.
3. **False.** They have the same power. A semaphore can be built as a
   monitor (exercise 2: a counter, a `while (value == 0) wait`, and a
   signal in `up`). A monitor can be built from semaphores (a semaphore
   with value 1 as the monitor lock, plus a semaphore and a counter for each
   condition). Anything one can do, the other can do.
4. **True** (this is the point of L09 slides 7–8). Mutual exclusion is
   automatic, so you cannot forget a lock or unlock. The data is private,
   so you only need to read the monitor's own code to check it. And waiting
   (condition variables) is separate from mutual exclusion. Monitors can
   still have bugs (a missing signal, `if` instead of `while`), so "less
   error-prone" does not mean "error-free".
5. **True.** Under signal and exit, the signaller leaves at once and the
   woken thread runs next, before anyone on the entry queue (L09 slide 15).
   Nobody can change `count` in between, so the condition is still true.
   (It is wrong under signal and continue.)
6. **False.** Under signal and continue, the woken thread has no priority
   (L09 slide 16). It joins the entry queue, and a thread already there
   may get in first ("barging") and make the condition false again.
7. **True.** POSIX allows *spurious wake-ups*: `pthread_cond_wait()` may
   return although nobody signalled. This is one reason the wait must be
   inside a `while` loop that re-tests the condition.
8. **False.** With `while` loops, every woken thread re-checks its
   condition and goes back to sleep if it is false. Extra wake-ups cost
   time but never let a thread go on with a false condition. (The reverse
   is not true: replacing a needed `broadcast` with `signal` can leave
   threads asleep for ever.)
9. **False.** A Java object has a single unnamed waiting queue (L09 slide
   21), used with `wait()`, `notify()` and `notifyAll()`. That is why Java
   monitors often need `notifyAll()`: producers and consumers wait in the
   same queue. (Java's newer `Lock`/`Condition` classes, L12 slides 45–47,
   do give separate named conditions.)
10. **False.** A monitor gives mutual exclusion, not freedom from deadlock.
    A thread can wait for a signal that never comes (all threads asleep),
    or a thread inside monitor M1 can call monitor M2 while another thread
    inside M2 calls M1: each waits for the other for ever.

## B. Lab 02 analysis questions

11. With `if`, a player that wakes up does not check `turn` again. In
    ping-pong there are only two threads, so a *real* signal always comes
    from the other player right after it set `turn = me`, and the output
    usually looks perfect. But a **spurious wake-up** (allowed by POSIX)
    would let a player return from the wait while it is still not its
    turn. It would print out of turn, e.g. `ping ping`. 100 good runs do
    not prove anything: the bad case is rare, and testing can only show
    that a bug exists, never that it does not. With `while`, the player
    checks `turn` again and goes back to sleep, so it is correct whatever
    wakes it.
12. It stays correct. At any time at most **one** thread can be waiting
    (the player whose turn it is not), so waking "all" waiters wakes that
    same one thread. Even if more threads were waiting, the `while (turn !=
    me)` re-check would send the wrong ones back to sleep. The only cost of
    broadcast is possible extra wake-ups.
13. Still **correct**: `put` waits while the 1 slot is full, `get` waits
    while it is empty. But the threads now **strictly alternate**: produce
    0, consume 0, produce 1, consume 1, ... (the printed lines can be a
    little out of step, because the printing happens outside the mutex). It
    is **slower** because almost every `put` and `get` has to sleep and be
    woken, which needs the operating system and a thread switch; with 5
    slots the producer can run ahead for a while and both threads sleep less
    often. On our test machine, 1 000 000 items took about 5.0 s with size 1
    and 0.9 s with size 5. Your numbers will differ; the direction will not.
14. (a) With one producer and one consumer, the producer only waits when
    the buffer is **full**, and the consumer only waits when it is
    **empty**. The buffer cannot be both at once (size ≥ 1). So at most one
    of them is ever waiting, and a signal always reaches the right thread.
    (b) One condition variable `c`, buffer size 1, `signal` may wake any
    waiter:
    1. C1 and C2 call `get`: the buffer is empty, both wait on `c`.
    2. P1 puts an item (count 1) and signals: C1 is woken (it now needs the mutex).
    3. P1 tries to put again: full, so P1 waits on `c`.
    4. P2 tries to put: full, so P2 waits on `c`.
    5. C1 gets the mutex, takes the item (count 0) and signals: this wakes
       **C2** (not a producer).
    6. C1 calls `get` again: empty, waits.
    7. C2 runs: count is 0, so it waits again.

    Now C1, C2, P1 and P2 are all asleep and nobody will ever signal: the
    program is stuck. The wake-up meant for a producer went to a consumer.
    With two condition variables, a consumer's signal on `not_full` can
    only wake a producer, so this cannot happen. (Using one condition
    variable with `broadcast` also works, but wakes everyone each time.)
15. With **two** condition variables (`not_full`, `not_empty`) and a signal
    after **every** `put` and **every** `get`, `signal` is correct: each
    change creates exactly one item or one free slot, which exactly one
    waiter of the right kind can use, and woken threads that lose the race
    re-check and sleep again. `broadcast` is needed (a) whenever producers
    and consumers share **one** condition variable (question 14), and (b)
    in lab track 3 when the **last sentinel** is taken: then *every* sleeping
    consumer must wake up and stop, not just one.
16. Yes. Items from different producers are mixed in the buffer in the
    order the producers happened to get the mutex, and with several
    consumers the moment each item is *processed or printed* depends on
    scheduling too. Items from the **same** producer enter the buffer in
    order, but with several consumers they may still be printed out of
    order. It is not a problem when items are independent (the lab only
    needs each item consumed exactly once). It would be a problem if the
    order carried meaning; then you would need one consumer or sequence
    numbers.
17. Consumers that are **asleep** in `get()` on an empty buffer are never
    woken. Example: 3 consumers, 3 producers. Consumer 1 takes the last
    sentinel, sees `sentinels_seen == 3` and stops. Consumers 2 and 3 are
    asleep waiting for an item that will never come, so `pthread_join`
    waits for ever. **Fix:** let `get()` know the number of producers. It
    waits only `while (count == 0 && sentinels_seen < M)`, returns "stop"
    (−1) when the buffer is empty and all sentinels were seen, and the
    consumer that takes the **last** sentinel calls `broadcast` so that all
    sleeping consumers wake up, see the new state and stop. (Another fix:
    after joining the producers, put one extra "stop" item per consumer.)
18. Find a time interval in which two or more readers have printed
    "started reading" but not yet "finished reading" (or the log shows
    `active_readers` > 1). To show no writer was active then, check that
    no "started writing" line lies between the first of those starts and
    the last of those finishes, or that every writer's start/finish pair
    lies completely outside that interval. Lab track 4's test replays its
    log in exactly this way, and prints the largest number of readers seen
    together.
19. `start_read()` only waits **while a writer is active**. It does not
    care whether a writer is waiting. With 5 readers that read, think a
    little, and read again, there is almost always at least one reader
    inside. New readers keep entering, so `reader_count` rarely drops to 0,
    and the writer (which needs `reader_count == 0`) keeps waiting. It only
    gets in when, by chance, all readers are thinking at the same time.
    This is **writer starvation** (readers' preference). With R = 1, W = 5
    the writers rarely wait long, because a single reader leaves often.
20. **Yes.** Once any writer is waiting, new readers must wait. If writers
    arrive often enough that `waiting_writers` never drops to 0 (a new
    writer always arrives before the current one finishes), readers wait
    for ever: **reader starvation**. A design without starvation for
    either side: (1) serve threads **in arrival order**, e.g. a ticket
    number or a single "turnstile" mutex that every thread passes through
    on arrival, so a waiting writer blocks *later* readers but not earlier
    ones; or (2) **alternate**: when a writer finishes, let in *all*
    readers that were waiting at that moment (a batch), and when that
    batch finishes, let in the next waiting writer. New readers that arrive
    while a writer waits join the *next* batch. Either way, every thread
    has only a bounded number of threads ahead of it.

## C. Traces

21.
    | Discipline | Order | Can A rely on its condition? |
    |---|---|---|
    | Signal and exit | B leaves at once (the signal is its last action), then **A**, then **C** | Yes: nobody runs between the signal and A |
    | Signal and wait | B pauses, **A** runs and leaves, then **B** finishes, then **C** | Yes: A runs immediately after the signal |
    | Signal and continue | **B** finishes its operation; then **A and C compete**: either B → A → C or B → C → A | No: B may change data after signalling, and C may get in first and use up what A waited for. A must re-check (`while`). |

22. (a) Signal and wait: B sets x = 1, prints `B 1`, signals, and A runs at
    once: prints `A 1`, sets x = 0, leaves. B resumes (nothing left) and
    leaves. C enters, sees x == 0 and waits. **Only output: `B 1 A 1`**
    (C stays asleep). (Signal and exit gives the same, because the signal is
    B's last action.)
    (b) Signal and continue: B prints `B 1`, signals (A moves to the
    entrance) and leaves. Now A and C compete:
    - A first: `A 1`, x = 0; C then waits. Output **`B 1 A 1`**.
    - C first: x is 1, so C does not wait: `C 1`, x = 0. Then A, which used
      `if`, does not re-check: `A 0`, x = **−1**. Output **`B 1 C 1 A 0`**.
      This is the bug: A went on although x == 0 (like taking from an empty
      buffer).

    (c) Change `if` to `while`. Now in the "C first" case A re-checks x == 0
    and goes back to sleep. Possible outputs: **`B 1 A 1`** (C asleep) or
    **`B 1 C 1`** (A asleep); x ends at 0 either way, never −1. (Checked
    with `tools/interleave`, modelling a wait as "release the lock; await
    a signal; re-lock": with `if` the outputs are exactly `B 1 A 1` and
    `B 1 C 1 A 0`; with `while`, `B 1 C 1 A 0` is impossible.)
23. 1. Consumers A and B find the queue empty and both wait on `notEmpty`.
    2. Producer C adds an item. The queue went from empty to non-empty, so
       C signals: A is woken (but has not got the lock yet).
    3. Producer D adds a second item. The queue was not empty, so D does
       **not** signal.
    4. A gets the lock and takes one item.
    5. One item is left in the queue, but B is still asleep, and no one
       will signal again.

    This is a **lost wake-up** (L12 slides 56–60). Fix: signal on every
    `enq` (or use `signalAll`, as L12 slide 61 says).
24. 1. Waiter: locks `m`, reads `ready == 0`, decides to wait.
    2. Signaller (no lock): sets `ready = 1`.
    3. Signaller: `cond_signal(c)`. Nobody is waiting on `c` yet, so the
       signal is lost.
    4. Waiter: calls `cond_wait(c, m)` and sleeps. Nobody will ever signal
       again: it sleeps for ever although `ready == 1`.

    If the signaller locked `m`, steps 2–3 could not happen between 1 and
    4, because the waiter holds `m` until `cond_wait` releases it while it
    starts sleeping. `break_it/lost_wakeup.c` (mode `nolock`) shows this.

## D. Code

25. ```c
    typedef struct { int value; pthread_mutex_t m; pthread_cond_t positive; } sem;
    void init(sem *s, int v) { s->value = v; /* + init m and positive */ }
    void down(sem *s) {
        lock(&s->m);
        while (s->value == 0) cond_wait(&s->positive, &s->m);
        s->value--;
        unlock(&s->m);
    }
    void up(sem *s) {
        lock(&s->m);
        s->value++;
        cond_signal(&s->positive);
        unlock(&s->m);
    }
    ```
    **Why `while`:** Pthreads uses signal and continue. Between the `up()`
    that woke a thread and the moment that thread gets the mutex back,
    another thread calling `down()` may take the permit (barging), and
    spurious wake-ups are allowed. The loop re-checks `value`.
    **Signal is enough:** one `up()` creates exactly one permit, which only
    one waiter can use. If the woken thread loses the permit to a barging
    thread, that thread consumed the permit, so no permit is left unused
    while someone sleeps.
26. ```c
    int barrier_wait(barrier *b) {
        lock(&b->m);
        unsigned long my_gen = b->generation;
        if (++b->arrived == b->n) {          // the last thread opens the barrier
            b->arrived = 0;                  // ready for the next round
            b->generation++;
            cond_broadcast(&b->all_here);
            unlock(&b->m);
            return 1;
        }
        while (b->generation == my_gen)      // wait until my round is over
            cond_wait(&b->all_here, &b->m);
        unlock(&b->m);
        return 0;
    }
    ```
    `while (arrived < n) wait;` fails because the last thread resets
    `arrived` to 0 (so the barrier can be reused) before the others wake
    up. They re-check, see 0 < n, and sleep for ever. If it does not reset,
    the barrier cannot be used a second time. The generation number changes
    exactly once per round, so a woken thread can tell its round has
    finished, even if fast threads have already arrived for the next round.
