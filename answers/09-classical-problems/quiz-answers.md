# 09 · Quiz answers: classical synchronization problems

These answers are about the length a marker expects for the marks shown.
"Why" is always part of the answer. Output and trace answers were checked
with `tools/interleave` (commands given).

## A. True / false

1. **False.** While readers keep arriving before the last reader leaves,
   `nr` never drops to 0, so `up(rw)` never happens and a waiting writer
   waits for ever (starvation). Slide 31 guarantees safety, not fairness to
   writers.
2. **False.** Each fork is a binary semaphore, so it is held by at most one
   philosopher. The deadlock is a **circle**: each philosopher holds one
   fork and waits for the fork its neighbour holds.
3. **False.** Philosophers 0–3 take fork i (left) first; philosopher 4 takes
   fork 0 first, which is its **right** fork. That breaks the circle: every
   philosopher takes its lower-numbered fork first.
4. **False.** `cutting` is declared but never used (section 7 of the
   README). There is only one barber thread, so he cannot cut two
   customers at once anyway.
5. **False.** It is readers' preference twice over: while cars keep coming
   from one side, that side's counter never drops to 0, so the bridge is
   never released to the other side. A turnstile fixes it (lab track 2).
6. **True.** Every valid group takes an even number of hackers and an even
   number of serfs, and 6 + 2 = 8 is two boats. Whatever forms first (4H, or
   2H + 2S), the 4 left over are 2H + 2S or 4H, which is valid. Check:
   `make run M=09 EX=lab4_river_crossing ANS=1 ARGS="6 2"`.

## B. Short answer

7. 1. P0 takes fork 0. 2. P1 takes fork 1. 3. P2 takes fork 2. 4. P3 takes
   fork 3. 5. P4 takes fork 4. 6. P0 waits for fork 1, P1 for fork 2, P2
   for fork 3, P3 for fork 4, P4 for fork 0. Every fork is held and every
   philosopher waits for a fork held by its neighbour: nobody can continue.
8. Every philosopher takes the **lower-numbered** of its two forks first
   (P4 takes fork 0 before fork 4). In a deadlock every philosopher would
   hold one fork and wait for a higher-numbered one held by the next
   philosopher. Following the circle, the fork numbers would have to
   increase all the way round and return to the start, which is
   impossible. Concretely, P0 and P4 compete for fork 0 first; the loser
   holds nothing, so it is not part of any circle, and its neighbour P3 can
   take both fork 3 and fork 4 and eat.
9. A writer is in the database only while it holds `rw`. The readers hold
   `rw` as a group: the first reader in (nr becomes 1) does `down(rw)`
   before reading, and the last reader out (nr becomes 0) does `up(rw)`.
   `nr` is changed and tested under `mutexr`, so "first" and "last" are
   decided correctly. Since `rw` starts at 1, it is held by either the
   reader group or one writer, never both at once.
10. R1 arrives at 0 and reads until 5. W arrives at 2 and does `down(rw)`:
    it waits, because the readers hold `rw`. R2 arrives at 3 and reads
    until 8: `nr` was 1, so R2 just joins. R1 leaves at 5 (`nr` = 1, no
    `up`). R3 arrives at 6, reads until 11; R2 leaves at 8. R4 arrives at
    9 … If a new reader always arrives before the last one leaves, `nr`
    never reaches 0, and W never gets `rw`.
11. ```c
    semaphore turnstile = 1, rw = 1, mutexr = 1;  int nr = 0;
    reader: down(turnstile); up(turnstile);
            down(mutexr); nr++; if (nr == 1) down(rw); up(mutexr);
            read;
            down(mutexr); nr--; if (nr == 0) up(rw); up(mutexr);
    writer: down(turnstile); down(rw); write; up(turnstile); up(rw);
    ```
    A waiting writer has already done `down(turnstile)`; it waits on `rw`
    while **holding** the turnstile. A reader arriving now blocks at
    `down(turnstile)`, so it cannot join the readers inside. Those readers
    finish, the last one does `up(rw)`, and the writer goes in. (Assumes the
    semaphores do not starve threads themselves.)
12. Add `semaphore room = 4;`. A philosopher does `down(room)` before
    picking up forks and `up(room)` after putting them down. So at most 4
    philosophers hold or want forks at once. With 4 philosophers and 5
    forks, if each held one fork, one fork would still be free; it lies
    next to one of the 4 (only one philosopher is outside), who can take it
    and eat. So the circle of all 5 waiting can never form.
13. `mutex` (1) protects the counter `waiting`, so two customers cannot both
    take the last chair. `customers` (0) counts customers waiting to be
    served; the barber sleeps on it when it is 0, and each customer who
    sits does `up(customers)`. `barber` (0) is the barber's "next, please":
    a seated customer waits on it, and the barber posts it once per
    customer he calls. Wrong or missing: `cutting` is declared but never
    used; there is no haircut step and the customer does not wait for the
    haircut to finish; customers loop and arrive again at once, instead of
    coming back later.

## C. Is this output possible?

14. (a) **Possible.** In slide 31 nothing stops a new reader: R2 sees that
    readers already hold the room (`nr` = 1) and joins, although the
    writer is waiting.
    (b) **Impossible.** `x` is printed after the writer closed the turnstile.
    R2 prints `y` after that and then must pass the turnstile, which stays
    closed until the writer has finished. So `b` must come after `W`.
    (`a x y A w W b B` is possible in both.)
    Check: `python3 tools/interleave/interleave.py modules/09-classical-problems/demo/q_rw_plain.il --why "a x y b A B w W"`
    and the same with `q_rw_turnstile.il`.
15. P0 takes f0, P1 takes f1, P2 takes f2 (each after leaving its
    non-critical section). Now P0 waits for f1, P1 for f2, P2 for f0. All
    three forks are 0 (taken), every philosopher holds exactly one fork and
    waits for the next. In m2, P2 takes f0 first: if P0 holds f0, P2 waits
    without holding anything, so P1 can take f2 and eat; if P2 holds f0, P0
    holds nothing, and so on. The explorer confirms `deadlock=none` for m2.
    Check: `make test M=09 T=m1_dining3_naive` and `T=m2_dining3_asymmetric`
    with the answers.

## D. Design and justification

16. ```c
    semaphore turnstile = 1;   // fairness: arrivals pass in order
    semaphore rope_free = 1;   // owned by the east group or the west group
    semaphore rope = 5;        // places on the rope
    int east = 0, west = 0;  semaphore mutex_e = 1, mutex_w = 1;   // two lightswitches
    baboon going east:
        down(turnstile);
        down(mutex_e); east++; if (east == 1) down(rope_free); up(mutex_e);
        up(turnstile);
        down(rope);  cross;  up(rope);
        down(mutex_e); east--; if (east == 0) up(rope_free); up(mutex_e);
    (west: the same with west / mutex_w)
    ```
    *Rule 1:* a baboon is on the rope only while its direction's group
    holds `rope_free` (first in locks, last out unlocks; `east` is tested
    and changed under `mutex_e`). `rope_free` starts at 1, so only one group
    holds it. *Rule 2:* `#on rope + rope = 5` and `rope ≥ 0`, so at most 5.
    *Rule 3:* a baboon that must wait for the other direction waits inside
    the lightswitch while holding the turnstile, so later baboons from its
    own side queue at the turnstile and cannot keep the rope busy. The
    baboons on the rope get off, the other direction takes over. The
    explorer checks rules 1–2 and no deadlock for a small version:
    `demo/baboons.il`.
17. Lab track 1 uses 4 semaphores: `bridge_free` (1: no direction owns the
    bridge), `capacity` (3: free places), and one mutex (1) inside each of
    the two lightswitches. You cannot do it with fewer than one semaphore
    per job: one for "which direction", one for "how many", and the
    counters need protecting. **Yes, a direction can starve**: while cars
    keep arriving from the north before the last northbound car leaves,
    the north lightswitch never reaches 0, so `bridge_free` is never
    released. Fix: a turnstile in front (lab track 2), which adds a 5th
    semaphore.
18. Larger N lets more trucks of the same direction cross together, so the
    total time drops: N = 1 is fully serial (about 30 × crossing time);
    N = 3 and N = 5 are faster. The gain shrinks because (a) only trucks
    going the same way can share, and with random arrivals there are often
    fewer than N of them waiting; (b) direction switches still force the
    zone to empty completely; (c) the arrival delays themselves put a
    lower limit on the total time. (Your measured table from
    `make run M=09 EX=lab2_construction_zone ARGS="table"` should show it.)
19. Changes: K barber threads all loop on `down(customers)`; the customer's
    id goes into a queue protected by the mutex; each customer waits on its
    **own** semaphore `done[id]`, which the barber that cut it posts. Why
    the one-barber design breaks: `barber_ready` and `cut_done` are shared.
    With two barbers, barber A may post `cut_done` for customer X, but
    customer Y (cut by barber B, not finished) takes that token and leaves
    mid-haircut. A token on a shared semaphore goes to whoever waits first,
    so "who is in which chair" is lost. Per-customer semaphores (or a
    per-barber chair semaphore) restore the pairing.
20. Only a **captain** posts the queue semaphores, and only after it has
    seen a valid group in the counters (4 of a kind, or 2 + 2); it posts
    exactly one token per seat and subtracts the group from the counters.
    The captain keeps the mutex until it has rowed, so meanwhile no other
    thread can change the counters or form a group, and the only threads
    waiting on the queues are the ones counted, so the 4 tokens go to group
    members. The captain waits for 3 `boarded` posts before rowing, so all
    4 are on board. **Deadlock:** with valid totals, no. Every thread waits
    either for the mutex (the captain always releases it after rowing) or
    on its queue (it is counted, and a group is formed as soon as the
    counters allow one). If the totals do not allow a final valid group
    (e.g. H = 5, S = 3), the last few wait for ever: not a cycle of
    threads holding resources, but waiting for partners who never come.
21. (a) yes (4H + 4S, or 2+2 twice); (b) yes (4H twice); (c) yes (question
    6); (d) **no**: after the first boat, 1H + 3S or 3H + 1S remain.
    Rule: everyone can cross exactly when H and S are both even and H + S
    is a multiple of 4. Why: every valid group takes an even number of each
    kind, so the parity of H and of S never changes; if both are even and
    the total is a multiple of 4, the last 4 are 4H, 4S or 2H + 2S, all
    valid. (Our solution forms a group as soon as one is possible, so it
    never gets stuck in a state where a valid group exists.)
22. More chairs, fewer turned away: with M = 1 many customers balk, with
    M = 10 far fewer (see `make run M=09 EX=lab3_barbershop ARGS="table"`).
    A customer counts as "waiting" until the barber calls it
    (`waiting--`). If all 20 arrive before the barber has called anyone,
    all 20 are waiting at once. So only **M = 20** guarantees that nobody
    is turned away. (M = 19 is often enough in practice, because the
    barber usually calls the first customer before the last ones arrive,
    but it is not guaranteed.)
23. The semaphore version is shorter, but its meaning is spread over the
    starting values and the order of the `down`s, so it is harder to read
    and easy to break with one misplaced line. With a mutex and condition
    variables, the condition is written out (`while (other_direction > 0
    || count == N) wait(...)`), so it is easier to see and to change the
    policy, e.g. "let the other side go after 5 trucks". So condition
    variables give more direct control over fairness; semaphores need
    extra patterns such as the turnstile.
