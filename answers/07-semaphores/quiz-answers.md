# 07 · Quiz answers: semaphores and synchronization patterns

These answers are about the length a marker expects for the marks shown.
"Why" is always part of the answer. Every output answer was checked with
`tools/interleave` (command given).

## A. True / false

1. **False** (by the lecture's definition). L08 slide 4: for a binary
   semaphore, `up` sets S = 1, so it stays 1. *Careful in C:* a POSIX `sem_t`
   has no binary mode, so `sem_post` on a value of 1 really makes it 2. An
   extra `sem_post` on a semaphore used as a lock then lets two threads in.
2. **False.** If some thread is asleep on S, `up` wakes one of them and S
   stays the same (the ticket goes straight to the woken thread). S only
   grows when nobody is asleep.
3. **False.** It is starvation-free with a **blocked-queue** semaphore
   (first come, first served). With a **busy-wait** semaphore a thread can
   starve even with 2 processes; with a **blocked-set** semaphore, with 3 or
   more (L08 slides 12–13).
4. **True.** That is the signalling pattern: the waiter does `down` and
   sleeps, because the value is 0. The other thread does `up` after its
   work, which wakes the waiter.
5. **False.** `spaces` only stops producers when the buffer is full. If
   there are 2 free slots, two producers both pass `down(spaces)` and can
   both write `buffer[tail]` before either moves `tail`. You need a mutex.
6. **False.** A fast thread can pass, call the barrier again, and take a
   gate token that was meant for a slow thread of the previous round. It
   then passes the next barrier before the others have arrived (question 21).
7. **True.** Readers keep the room locked as long as at least one reader is
   inside. If new readers keep arriving before the last one leaves, the
   counter never drops to 0, `up(rw)` never happens, and the writer waits
   forever. Every individual reader finishes, but the group never does.

## B. Short answer

8. (a) Invariant 2: S = S0 + #up − #down = 2 + 6 − 7 = **1**.
   (b) **No.** It would give S = 2 + 6 − 9 = −1, which breaks invariant 1
   (S ≥ 0). At most 8 `down`s can have finished; a 9th thread would still be
   asleep inside its `down`.
9. A thread is in the CS exactly when it has finished `down(S)` and not yet
   done `up(S)`. So `#CS = #down − #up`. Invariant 2 with S0 = 1 gives
   `S = 1 + #up − #down`. Adding: `#CS + S = 1`. This holds after every step,
   because both equations do. Since S ≥ 0 (invariant 1), `#CS = 1 − S ≤ 1`:
   at most one process is in its critical section.
10. A deadlock would mean all processes are asleep on `down(S)`. Then S = 0
    (otherwise `down` would not sleep) and nobody is in the CS, so
    `#CS = 0`. Then `#CS + S = 0`, which contradicts `#CS + S = 1`. So a
    deadlock is impossible.
11. 1. P1: `down(S)` succeeds (S = 0); P1 enters.
    2. P2: checks S, it is 0; P2 loops.
    3. P1: leaves, `up(S)` (S = 1), runs its non-critical section,
       `down(S)` (S = 0), enters again.
    4. P2: checks S, it is 0 again; loops.
    Steps 3–4 repeat forever. P2 is never woken, because a busy-wait `up`
    only adds 1. (The explorer finds this loop in model m2.)
12. P1 is in the CS, P2 and P3 are asleep. P1's `up` wakes P3; P1 comes back
    and sleeps. P3's `up` wakes P1; P3 comes back and sleeps. Repeat: P1 and
    P3 take turns forever, and P2 is never chosen, because blocked-set `up`
    may wake **any** sleeper. With a blocked-queue semaphore, `up` wakes the
    thread at the front of the queue. P2 has at most N − 1 = 2 threads ahead
    of it, so it is woken after at most 2 `up`s.
13. Only the producer writes `tail` and only the consumer writes `head`, so
    no variable is written by both. The consumer reads a slot only after
    `up(elements)` (or `tail > head`) says it is filled, and the buffer is
    infinite, so the producer never writes into a slot the consumer is
    reading. With several producers this would no longer be true.
14. Buffer with N = 4, empty, `tail = 0`.
    1. Producer A: `down(spaces)` (4 → 3).
    2. Producer B: `down(spaces)` (3 → 2).
    3. A: `buffer[tail] = x` (slot 0).
    4. B: `buffer[tail] = y` (slot 0 again: **x is lost**).
    5. A: `tail = 1`. 6. B: `tail = 2`. 7. Both do `up(elements)`.
    The consumers now take `y` from slot 0 and an old, never-written value
    from slot 1. (The `tail = tail + 1` steps can also overlap and lose an
    update, as in module 01.)
15. **Deadlock.** If the buffer is full, a producer takes the mutex, then
    sleeps on `down(spaces)` **while holding the mutex**. A consumer, which
    is the only thread that could free a slot, blocks on `down(mutex)`.
    Nobody can move. (The same happens to a consumer on an empty buffer.)
    Fix: wait for the slot or the item **first**, then take the mutex just
    around the buffer access:
    `down(spaces); down(mutex); store; move tail; up(mutex); up(elements);`
    and the mirror image in the consumer.

## C. Is this output possible?

16. **Possible: `A B C` and `C B A`. Impossible: `A C B`, `B A C`.** Exactly
    **2** outputs. Why: B needs a ticket on `s1`, which starts at 0, so B
    always comes after A or C. A and C both need `s2`, which starts at 1, so
    only one of them can go first. The other must wait for B's `up(s2)`. So
    the order is always "A or C, then B, then the other one".
    Check: `python3 tools/interleave/interleave.py modules/07-semaphores/demo/q_three_threads.il --outputs`
17. (a) **Possible.** Both readers read together, then the writer writes.
    The first reader in locks the room; the second just joins; the last
    one out unlocks it.
    (b) **Impossible.** After `a`, R1 is reading, so the room is locked for
    readers. The writer's `down(room)` cannot finish until the last reader
    has left, so `w` cannot come before `A`.
    (c) **Possible.** The writer goes first. The first reader to arrive
    waits on `down(room)` (still holding the counter's mutex), and the other
    reader waits for that mutex. When the writer leaves, both read together.
    (d) **Impossible.** After `b a`, both readers are inside. `w` needs both
    to have left, but `B` has not been printed yet.
    The program has 14 possible outputs; check with
    `python3 tools/interleave/interleave.py modules/07-semaphores/demo/q_lightswitch_order.il --why "a b B A w W"`
    (and the same with the other three).
18. One order (found by the explorer):
    1. R1: `down(mutex)`, `readers = 1`, `up(mutex)`.
    2. R2: `down(mutex)`, `readers = 2`, `up(mutex)`.
    3. R1 tests `readers == 1`: false (it is 2), so R1 does **not** lock the room. R1 prints `a`.
    4. R2 tests `readers == 1`: false, so R2 does not lock the room either.
    5. W: `down(room)` succeeds (room = 0). W prints `w`. **A reader and the writer are inside together.**
    6. R1 prints `A`; R1 leaves: `readers = 1`, not 0, so no `up`.
    7. W prints `W`.
    8. R2 prints `b`, `B`; R2 leaves: `readers = 0`, so `up(room)` (room = 1).
    9. W: `up(room)` (room = **2**).
    At the end `room = 2`: a semaphore meant to be binary now holds two
    tickets, so later **two writers** could be inside together. The cause in
    both cases: the test "am I the first?" must happen under the same mutex
    as the update of `readers`. (The buggy version can also deadlock.)
    Check: `python3 tools/interleave/interleave.py modules/07-semaphores/demo/q_lightswitch_bug.il --why "a w A W b B"`
19. **`p p p c c c` is impossible:** `spaces` starts at 2, so the third `p`
    needs a `c` first. The 4 possible outputs are `p c p c p c`,
    `p c p p c c`, `p p c c p c`, `p p c p c c`. In each, at every point,
    (number of p) − (number of c) is between 0 and 2.
    Check: `python3 tools/interleave/interleave.py modules/07-semaphores/demo/q_buffer_two_slots.il --outputs`
20. | thread | arrives | starts | finishes |
    |---|--:|--:|--:|
    | R1 | 0 | 0 | 10 |
    | W | 2 | **13** | 16 |
    | R2 | 4 | 4 | 6 |
    | R3 | 9 | 9 | 13 |

    R1 locks the room for readers at 0. W waits. R2 and R3 arrive while a
    reader is inside, so they join straight away, even though W has been
    waiting longer. The room is first empty at 13 (R3 leaves), so W writes
    from 13 to 16. This shows **readers' preference**: a writer can be
    overtaken by later readers, and with a steady stream of readers it could
    wait forever (starvation).

## D. Lab-style and code

21. Naive barrier: `down(mutex); count++; if (count == n) { count = 0; up(gate) n times; } up(mutex); down(gate);`
    Failure (n = 2): the slow thread S arrives last and posts 2 tokens. The
    fast thread F takes one token, does its next step, and calls the barrier
    again: `count = 1`, then `down(gate)` finds the **second token** (meant
    for S) and passes. F has passed barrier k+1 although S has not even left
    barrier k. Then S finds no token and sleeps, possibly forever.
    Two-phase fix: after the first gate, every thread counts out again, and
    the last one out opens a second gate. A thread can only return to gate 1
    after passing gate 2, and gate 2 opens only when all n threads have
    taken their gate-1 token. So no token of an old round is left for it to
    steal.
22. (a) Each step has a "read old values, compute temp" part and a "write
    new values into arr" part. Barrier 1 (after computing): nobody writes
    `arr` while a neighbour thread is still reading the old values. Barrier
    2 (after copying): nobody starts the next step and reads a neighbour's
    element before that neighbour has written its new value. Without either
    one, a thread can mix old and new values.
    (b) **Exactly the same.** Every element is computed with the same
    formula, from the same inputs, with the same operations in the same
    order as in the serial code. Only the thread that does it changes.
    Floating-point differences would appear only if the order of the
    additions changed (for example in a parallel sum), because floating-point
    addition is not associative.
23. ```c
    int buffer[N], head = 0, tail = 0;
    semaphore spaces = N;    // free slots: producers wait on it
    semaphore elements = 0;  // filled slots: consumers wait on it
    semaphore mutex = 1;     // one thread at a time touches buffer/head/tail

    producer: while (true) {
        item = produce();
        down(spaces); down(mutex);
        buffer[tail] = item; tail = (tail + 1) % N;
        up(mutex); up(elements);
    }
    consumer: while (true) {
        down(elements); down(mutex);
        item = buffer[head]; head = (head + 1) % N;
        up(mutex); up(spaces);
        consume(item);
    }
    ```
    The mutex is needed because there are several producers and several
    consumers. It goes **inside** the `spaces`/`elements` waits (question 15).
24. ```c
    semaphore soup_ready = 0;        // the slides call it "first"
    semaphore sandwiches_ready = 0;  // "second"
    SYNCH 1 (Bill): up(sandwiches_ready); down(soup_ready);
    SYNCH 2 (Ben):  up(soup_ready);       down(sandwiches_ready);
    ```
    Bill cannot pass `down(soup_ready)` until Ben has done `up(soup_ready)`,
    which Ben does only after making soup; and the same the other way. If
    both did `down` first: Bill sleeps on `soup_ready` (0), Ben sleeps on
    `sandwiches_ready` (0). The only `up`s that could wake them come after
    those `down`s, so neither ever runs: deadlock. (The explorer shows this
    in model m1: every run gets stuck after `s p`.)
25. A **multiplex**: `semaphore slots = 20;` and each request runs
    `down(slots); handle_request(); up(slots);`. Invariant:
    `#handling + slots = 20` (the same argument as `#CS + S = 1`: a request
    is handling exactly when it has finished `down` but not done `up`).
    Since `slots ≥ 0`, `#handling ≤ 20`. The 21st request sleeps in `down`
    until one finishes.
