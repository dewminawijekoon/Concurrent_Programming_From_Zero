# 07 · Semaphores and synchronization patterns ★

| | |
|---|---|
| **Time** | about 5 hours |
| **You should know first** | module 03 (starting threads in C) and module 05 (critical sections, the four rules, deadlock and starvation) |
| **Lectures** | L08 (semaphores) slides 1–25; L09 *Basic Constructs II* slides 3–5, 27–28 |
| **Helps with** | Lab 04 Ex 5 (reusable barrier); Lab 04 Ex 1–4 via module 09; exam questions like "is this output possible?", "prove mutual exclusion", "write the producer and the consumer" |

**Goal:** use one small tool, the semaphore, to make threads take turns,
wait for each other, and share a buffer safely. You will learn a handful of
standard patterns that solve most exam questions on semaphores.

Words in **bold** link to the [glossary](../../GLOSSARY.md) the first time
they appear.

---

## 1. The problem

In module 05 you built locks from plain reads and writes. They work, but
they have two problems.

1. **They waste the CPU.** A waiting thread runs `while (...) {}` again and
   again. This is [**busy-waiting**](../../GLOSSARY.md#busy-waiting-spinning).
   On a single core it can even stop the program: if a high-priority
   thread spins while a low-priority thread holds the lock, the low-priority
   thread never runs and never lets go (L09 slide 3).
2. **They only do one job.** A lock says "one at a time". But threads often
   need to say "**wait until** something has happened". Example: a printer
   thread must wait until there is a page to print. That is a different
   kind of waiting, called *condition synchronization*.

We want one tool that does both jobs, and that lets a waiting thread
*sleep* instead of spinning. Dijkstra invented it in the 1960s (L08 slide 2).

## 2. The idea: a semaphore

A **semaphore** is a shared counter that can never go below zero. Think of a
bowl of tickets. The number of tickets in the bowl is the semaphore's
value.

| Operation | Plain meaning | Other names |
|---|---|---|
| `down(S)` | Take a ticket. If the bowl is empty, **sleep** until someone puts one back. | P(S), wait(S), `sem_wait(&s)` |
| `up(S)` | Put a ticket back. If someone is sleeping on S, wake one of them. | V(S), signal(S), `sem_post(&s)` |

The lecture's definition (L08 slide 3), in plain words:

```
down(S):  if S > 0, then S = S - 1;
          otherwise the calling thread is suspended (it sleeps) on S.
up(S):    if some thread is suspended on S, wake one of them;
          otherwise S = S + 1.
```

Three rules come with it (L08 slide 4):

- `down` and `up` are each [**atomic**](../../GLOSSARY.md#atomic). Nobody
  can squeeze in between "check S > 0" and "S = S − 1". (In module 05 this
  gap was exactly what broke Attempt 2.)
- A semaphore must start at a value of 0 or more.
- `up` wakes **one** sleeping thread. The definition does **not** say which
  one. Remember this; it matters for starvation (section 3.3).

**Two kinds.**
- A *general* (or *counting*) semaphore can hold any value 0, 1, 2, …
  Example: a car park with 5 spaces starts at 5.
- A *binary* semaphore only holds 0 or 1. Example: a semaphore used as a
  lock starts at 1.

**A semaphore is not owned by anyone.** A [**lock (mutex)**](../../GLOSSARY.md#lock-mutex)
should be unlocked by the thread that locked it. A semaphore can be `up`ped
by *any* thread, even one that never did `down`. That is what makes
semaphores good for *signalling* between threads (Lab 04, quick reference).

## 3. How semaphores are used

### 3.1 Mutual exclusion with one semaphore (L08 slides 6–8)

```c
semaphore S = 1;              // one ticket: "the critical section is free"
// every thread:
while (true) {
    non_critical_section();
    down(S);                  // take the ticket (sleep if it is gone)
    critical_section();
    up(S);                    // give it back
}
```

A run with two threads:

| step | P1 does | P2 does | S | who is in the CS |
|--:|---|---|:--:|---|
| 0 | | | 1 | nobody |
| 1 | `down(S)`: S was 1, now 0 | | 0 | P1 |
| 2 | | `down(S)`: S is 0, so P2 **sleeps** | 0 | P1 |
| 3 | `up(S)`: P2 is asleep, so wake P2 (S stays 0) | | 0 | P2 |
| 4 | | `up(S)`: nobody asleep, so S = 1 | 1 | nobody |

At step 3, notice that S stays 0. The ticket goes straight to P2.

This works for any number of threads, not just two (L08 slide 7).

### 3.2 Why it is correct: invariants (L08 slides 5, 7, 8)

An **invariant** is a statement that is true at the start and stays true
after every step. To prove a safety rule, you find an invariant that makes
the bad situation impossible.

**Two invariants of every semaphore** (slide 5). Let S0 be the starting
value, `#up` the number of `up` operations done so far, and `#down` the
number of `down` operations that have *finished*.

```
(1)  S >= 0
(2)  S = S0 + #up - #down
```

Why (2) holds: think of `up` as "add a ticket" and a finished `down` as
"remove a ticket". At step 3 above, `up` handed its ticket straight to P2:
that counts as one `up` plus one finished `down` (P2's), so S does not
change. The equation still balances.

**Mutual exclusion** (slide 7). Let `#CS` be the number of threads inside
the critical section. A thread is inside exactly when it has finished its
`down` but not yet done its `up`. So

```
#CS = #down - #up
S   = 1 + #up - #down        (invariant 2 with S0 = 1)
add the two lines:   #CS + S = 1          (3)
```

Since S ≥ 0 (invariant 1), `#CS ≤ 1`. So at most one thread is inside.

**No deadlock** (slide 8). Suppose both threads were asleep on `down(S)`.
Then S = 0 and nobody is inside, so `#CS + S = 0`. That breaks invariant (3).
So it cannot happen.

**No starvation for two threads** (slide 8). If P1 is asleep, then S = 0,
so by (3) P2 is inside. When P2 leaves, its `up` must wake a sleeping
thread, and P1 is the only one. So P1 gets in.

**Works without competition** (slide 8). If nobody else wants in, S = 1,
and `down` does not wait.

### 3.3 Three kinds of semaphore, and starvation (L08 slides 9–13)

Different books define the semaphore slightly differently. The difference
decides whether a thread can [**starve**](../../GLOSSARY.md#starvation).

| Kind | `down` when S = 0 | `up` when threads wait | Starvation possible? |
|---|---|---|---|
| **Blocked-set** (slide 9) | sleep | wake **any one** sleeping thread | no for 2 threads; **yes for 3 or more** |
| **Blocked-queue** (slide 10) | sleep, joining a first-come-first-served queue | wake the thread at the **front** of the queue | **no** |
| **Busy-wait** (slide 11) | keep checking in a loop: "if S > 0 then S = S − 1" (each check is atomic) | just S = S + 1; nobody is "woken" | **yes**, even for 2 threads |

**Busy-wait: starvation with two threads** (slide 12).
1. P1 does `down(S)` and enters.
2. P2 checks S: it is 0. P2 loops.
3. P1 leaves (`up`, S = 1), does its non-critical work, comes back and
   does `down(S)` (S = 0) again, all between two of P2's checks.
4. P2 checks S: it is 0 again. Repeat forever.

P2 is never woken, because nobody wakes anyone in a busy-wait semaphore.
It just has bad luck every time it looks. Model exercise m2 shows the
explorer finding exactly this loop.

**Blocked-queue: no starvation** (slide 13). If P1 is in the queue, at
most N − 1 threads are ahead of it. Each `up` moves the queue forward by
one. So P1 gets in after at most N − 1 `up`s.

**Blocked-set: starvation with three threads.** The slide leaves this as an
exercise. Here is one way. Remember `up` may wake *any* sleeping thread.
1. P1 is inside. P2 and P3 are asleep on S.
2. P1 does `up`, and it wakes P3. P1 comes back and goes to sleep on S.
3. P3 does `up`, and it wakes P1. P3 comes back and goes to sleep on S.
4. Go back to step 2.

P1 and P3 take turns forever. P2 is always asleep but never chosen.

### 3.4 Patterns: small recipes you will reuse

Most semaphore problems are built from a few standard pieces. The names
below come from Allen Downey's free book *The Little Book of Semaphores*
[EXTRA: the book is not part of the notes, but the patterns are]. For each
one, read the code and the "why" line.

**Signalling: "A must happen before B".** One thread does A, another does B.

```c
semaphore a_done = 0;
thread 1:   A;  up(a_done);
thread 2:   down(a_done);  B;    // sleeps until A is done
```
*Why:* the semaphore starts at 0, so `down` cannot finish until the `up`
has happened, and the `up` comes after A.

**Rendezvous: "both wait for each other".** Thread 1 does a1 then a2;
thread 2 does b1 then b2. We need a1 before b2 **and** b1 before a2. This
is the Bill and Ben exercise (L08 slides 14–18; exercise 1).

```c
semaphore a_ready = 0, b_ready = 0;
thread 1:   a1;  up(a_ready);  down(b_ready);  a2;
thread 2:   b1;  up(b_ready);  down(a_ready);  b2;
```
*Why:* each thread first says "I'm ready", then waits for the other.
If **both** threads did `down` first, each would wait for an `up` the
other never reaches: [**deadlock**](../../GLOSSARY.md#deadlock). (One thread
may do `down` first, as long as the other does `up` first. The slides show
both of these correct versions.)

**Mutex.** A semaphore that starts at 1 (section 3.1).

**Multiplex: "at most N inside".** A semaphore that starts at N (L09 slide
27; exercise 2).

```c
semaphore tickets = N;
every thread:   down(tickets);  /* at most N threads here */  up(tickets);
```
*Why:* `#inside + tickets = N` is an invariant (the same proof as 3.2), and
tickets ≥ 0, so `#inside ≤ N`.

**Lightswitch: "the first one in locks, the last one out unlocks".** Used
for readers and writers (L08 slide 31; exercise 3).

```c
int count = 0;  semaphore count_mutex = 1;   // one lightswitch
lightswitch_lock(room):
    down(count_mutex);
    count = count + 1;
    if (count == 1) down(room);   // first in: lock the room for the whole group
    up(count_mutex);
lightswitch_unlock(room):
    down(count_mutex);
    count = count - 1;
    if (count == 0) up(room);     // last out: unlock the room
    up(count_mutex);
```
Readers call the lightswitch; writers do `down(room)` / `up(room)` directly.

*Why:* only the first reader competes with writers for the room. Later
readers just join, because the room is already "theirs". The counter needs
its own mutex: if two readers could change `count` at the same time, both
might think they are first (or neither might), and the room breaks. The
quiz has a puzzle about exactly that bug.

*Price:* while readers keep arriving, `count` never drops to 0, so a
waiting writer may wait forever. Readers are preferred; writers can starve.
Module 09 fixes this with a turnstile.

**Turnstile: "let threads through one at a time".**

```c
down(turnstile);  up(turnstile);
```
*Why:* if the turnstile is 1 (open), each thread passes straight through.
If someone holds it at 0 (locked), everyone queues up there. Module 09 uses
this to stop new readers while a writer is waiting.

**Barrier: "nobody goes on until all N have arrived"** (L09 slide 28). See
section 3.6.

### 3.5 Producer and consumer (L08 slides 19–25)

A *producer* thread makes data items; a *consumer* thread uses them.
Between them sits a *buffer*: a queue. The producer adds at the `tail`; the
consumer takes from the `head`.

**Infinite buffer** (slides 21–23). Pretend the buffer never fills up. Then
only the consumer ever has to wait, and only while the buffer is empty. One
semaphore `elements`, starting at 0, counts the items in the buffer:

```c
producer:  item = produce();  buffer[tail] = item;  tail++;  up(elements);
consumer:  down(elements);  item = buffer[head];  head++;  consume(item);
```

Slide 21 asks: *why is mutual exclusion not an issue here?* Because with
**one** producer and **one** consumer, they never touch the same variable
at the same time in a harmful way. The producer is the only one writing
`tail`, and the consumer is the only one writing `head`. The consumer only
reads a slot after `up(elements)` has said "it is filled", so it never
reads a half-written slot.

**Circular buffer with N slots** (slides 24–25). Now the producer must also
wait when the buffer is full. Add a second counter, `spaces`, that starts
at N:

```c
semaphore elements = 0, spaces = N;
producer:  down(spaces);    buffer[tail] = item;  tail = (tail + 1) % N;  up(elements);
consumer:  down(elements);  item = buffer[head];  head = (head + 1) % N;  up(spaces);
```

A run with N = 2 (✓ = the semaphore had a ticket, ✗ = the thread sleeps):

| step | producer | consumer | elements | spaces |
|--:|---|---|:--:|:--:|
| 0 | | | 0 | 2 |
| 1 | `down(spaces)` ✓, store item 1, `up(elements)` | | 1 | 1 |
| 2 | `down(spaces)` ✓, store item 2, `up(elements)` | | 2 | 0 |
| 3 | `down(spaces)` ✗ sleeps: buffer full | | 2 | 0 |
| 4 | | `down(elements)` ✓, take item 1, `up(spaces)` wakes producer | 1 | 0 |
| 5 | store item 3, `up(elements)` | | 2 | 0 |

**More than one producer: you need a mutex.** Suppose two producers both
pass `down(spaces)` at the same time:

| step | producer A | producer B | tail |
|--:|---|---|:--:|
| 1 | `buffer[tail] = x` (writes slot 0) | | 0 |
| 2 | | `buffer[tail] = y` (also writes slot 0: **x is lost**) | 0 |
| 3 | `tail = (tail+1) % N` | | 1 |
| 4 | | `tail = (tail+1) % N` | 2 |

Slot 1 was never filled, but `elements` says two items are there, so a
consumer will read rubbish from slot 1. The same can happen with two
consumers and `head`. The fix is a third semaphore `mutex = 1` around the
lines that touch the buffer and the index:

```c
producer:  down(spaces);  down(mutex);  store at tail; move tail;  up(mutex);  up(elements);
consumer:  down(elements); down(mutex); take at head; move head;   up(mutex);  up(spaces);
```

**The order matters.** Do `down(spaces)` **before** `down(mutex)`. If you
took the mutex first and the buffer was full, the producer would sleep
*while holding the mutex*. No consumer could then get the mutex to make
room. Deadlock. Rule: never go to sleep on a semaphore while you hold a
mutex that the waker needs.

### 3.6 Barriers, and why the simple one cannot be reused (L09 slide 28; Lab 04 Ex 5)

A **barrier** for n threads: each thread calls `barrier_wait()`, and nobody
returns from it until all n have called it. A typical use is a loop where
step k+1 needs the results of step k from every thread, like the
averaging in Lab 04 Ex 5.

**A one-time barrier** (it works once):

```c
int count = 0;  semaphore mutex = 1, gate = 0;
barrier_wait():
    down(mutex);  count = count + 1;
    if (count == n) { for (i = 0; i < n; i++) up(gate); }   // last one opens the gate n times
    up(mutex);
    down(gate);                                              // wait at the gate
```

**Why it breaks when reused** (Lab 04 Q5.1). Suppose the last thread also
resets `count = 0` so the barrier can be used again. With n = 2:

| step | fast thread F | slow thread S | gate |
|--:|---|---|:--:|
| 1 | arrives, count = 1, waits at gate | | 0 |
| 2 | | arrives, count = 2, last: count = 0, `up(gate)` twice | 2 |
| 3 | takes a token, passes, works, calls the barrier **again**: count = 1 | | 1 |
| 4 | `down(gate)`: a token is there, so F **passes the second barrier alone** | | 0 |
| 5 | | `down(gate)`: no token left, S sleeps (stuck in the first barrier) | 0 |

F took the token that was meant for S. F is now one step ahead, which is
exactly what a barrier must prevent. Then S may stay stuck forever. Run
`make run M=07 EX=naive_barrier` to watch it happen on your machine.

**The fix: two phases (a reusable barrier).** Use two gates.

```c
barrier_wait():
    // phase 1: arrive
    down(mutex);  count++;
    if (count == n) up(gate1) n times;      // last to arrive opens gate 1
    up(mutex);
    down(gate1);
    // phase 2: leave
    down(mutex);  count--;
    if (count == 0) up(gate2) n times;      // last to leave opens gate 2
    up(mutex);
    down(gate2);
```

*Why it works:* to come back to gate 1, a fast thread must first get
through gate 2. Gate 2 opens only after **all** n threads have counted out
in phase 2, and each of them took its gate-1 token before that. So when the
fast thread returns to gate 1, every token of the last round is gone. It
must wait for the next "last thread". The same argument, the other way
round, protects gate 2. You write this in lab track 5.

## 4. Why it works: the proof habits to copy

- **Counting invariants.** For any semaphore: `S = S0 + #up − #down` and
  `S ≥ 0`. Add the number of threads "holding a ticket" and you get an
  invariant like `#CS + S = 1` (mutex) or `#inside + S = N` (multiplex).
- **Bounded buffer.** When no thread is in the middle of a put or a get,
  `elements + spaces = N`. (A producer that has done `down(spaces)` but not
  yet `up(elements)` holds one "ticket" in its hand for a moment.) `elements` goes up only after an item is
  stored, and a consumer must take one of those tickets before it reads, so
  the consumer never reads a slot that has not been filled. Since `spaces ≥ 0`, the producer is never
  more than N items ahead of the consumer, so it never overwrites an item
  that has not been taken.
- **"Can it deadlock?"** Assume every thread is asleep. Write down what the
  semaphore values must then be. Show that this contradicts an invariant.
- **"Can it starve?"** Name the semaphore kind first (section 3.3). With a
  blocked-queue semaphore, count how many threads can be ahead of you.

## 5. API cheat-sheet (POSIX, `#include <semaphore.h>`, link with `-pthread`)

The notes use `down`/`up` pseudocode. In C you use the POSIX names
[EXTRA: the C functions are in Lab 04, not in the lecture notes].

| Call | Meaning |
|---|---|
| `sem_t s;` | a semaphore variable |
| `sem_init(&s, 0, value)` | set it up with a starting value. The `0` means "shared between the threads of this process" (not between processes). |
| `sem_wait(&s)` | `down`: take one, or sleep until you can |
| `sem_post(&s)` | `up`: give one back, waking a sleeper if there is one |
| `sem_trywait(&s)` | take one if the value is above 0 (returns 0); otherwise return −1 at once, without waiting |
| `sem_destroy(&s)` | clean up when no thread uses it any more |

**macOS:** Apple does not support these *unnamed* semaphores
(`sem_init` fails there). Use the dev container (see the top-level README).

Two differences from the lecture model:
- POSIX does not say which sleeping thread `sem_post` wakes, and a thread
  that arrives just then may take the ticket first. So do not rely on
  first-come-first-served order.
- `sem_wait` can return early with an error if a signal interrupts it. In
  these exercises that does not happen, so we do not check for it.

## 6. Common exam mistakes

- Starting a signalling semaphore at 1 instead of 0. Then the "wait" passes
  before anything has happened.
- Rendezvous with `down` before `up` in **both** threads: deadlock.
- In the bounded buffer, taking the mutex **before** `down(spaces)` or
  `down(elements)`: deadlock when the buffer is full or empty.
- Saying the slide 25 buffer (two semaphores) works for several producers.
  It does not: two producers can write the same slot. It works for one
  producer and one consumer.
- Writing `if (count == 1)` in the lightswitch **outside** the counter's
  mutex. Two readers can both see 2, so nobody locks the room, and a writer
  gets in with readers.
- Claiming "a semaphore solution cannot starve". It depends on the kind of
  semaphore (section 3.3): busy-wait can starve with 2 threads, blocked-set
  with 3.
- Forgetting that `up` on a blocked-set semaphore with sleepers does **not**
  increase S. In a value trace, write "wakes P2, S stays 0".
- Saying the lightswitch solution is fair. Writers can starve.
- In a barrier, forgetting the reuse problem: one counter and one gate
  only works once.

## 7. Careful: what the notes say

- **L08 slide 5** asks you to "show" the two invariants but gives no proof.
  Section 3.2 gives one.
- **L08 slide 13** leaves the blocked-set starvation example for N > 2 as
  an exercise. Section 3.3 gives one.
- **L08 slides 16–18** show three versions of the Bill and Ben solution
  with the unhelpful names `first` and `second`. All three are correct.
  `first` means "Ben's soup is ready"; `second` means "Bill's sandwiches
  are ready". The wrong version (both threads `down` first) is not shown;
  model m1 shows that it deadlocks.
- **L08 slide 22** lists the infinite-buffer invariants in a garbled form
  ("#E = 0+tail-head", "E>=0"). It means: the number of items in the buffer
  is `tail − head`, and that is never negative.
- **L08 slide 24** says one of the jobs is "only one process at a time
  manipulates a buffer slot", but the code on slide 25 has no mutex. That
  code is correct only for **one** producer and **one** consumer. With more,
  add a mutex (section 3.5). This is also in [NOTES_ERRATA.md](../../NOTES_ERRATA.md).
- **L09 slide 28** (barrier) is only pictures, with no code. Section 3.6
  and lab track 5 fill this in.
- **Names.** The notes say `down`/`up`; Lab 04 says `sem_wait`/`sem_post`
  and P/V; L09 says the counter "goes up and down". They are all the same
  two operations.

## 8. [EXTRA] Going further

- **Semaphore or mutex?** In C, use `pthread_mutex_t` for "one at a time"
  (it has an owner, so tools can catch mistakes like unlocking from the
  wrong thread) and a semaphore for counting and signalling.
- **Why semaphores are called "low level"** (L09 slide 7): one missing or
  extra `up`/`down` anywhere in the program breaks everything, and you must
  read the whole program to see how a semaphore is used. Monitors and
  condition variables (module 08) group the data and its synchronization
  together.
- **Waiting with a time limit:** `sem_timedwait(&s, &deadline)` gives up
  at a clock time. `break_it/naive_barrier.c` uses it so the demo cannot
  hang.
- **Further reading:** Allen B. Downey, *The Little Book of Semaphores*
  (free online). Chapters 3–4 match this module and module 09.

## 9. Now do

| # | Exercise | Type | What you practise |
|---|---|---|---|
| 1 | `exercises/concept/ex1_bill_and_ben.c` | warm-up | rendezvous with two semaphores (L08 slides 14–18) |
| 2 | `exercises/concept/ex2_multiplex.c` | warm-up | a counting semaphore; `sem_trywait` |
| 3 | `exercises/concept/ex3_lightswitch.c` | core | the lightswitch; a readers/writers room |
| 4 | `exercises/concept/ex4_bounded_buffer.c` | core | producer/consumer with `spaces`, `elements`, then `mutex` |
| 5 | `exercises/concept/ex5_bounded_buffer_from_memory.c` | from memory | write the three-semaphore buffer without looking |
| L5 | `exercises/lab/lab5_reusable_barrier.c` | lab track | **Lab 04 Ex 5**: two-phase barrier + parallel iterative averaging, checked against the serial result |
| m1 | `exercises/models/m1_bill_and_ben.il` | model | prove the rendezvous right for every order of steps; see the deadlocking version |
| m2 | `exercises/models/m2_busy_wait_starvation.il` | model | see why a busy-wait semaphore can starve a thread |
| demo | `break_it/naive_barrier.c` | demo | watch the one-gate barrier break when reused |

```bash
make test M=07                                # test all exercises
make test M=07 T=ex4_bounded_buffer           # test one exercise
make run  M=07 EX=ex3_lightswitch             # run your code on its own
make run  M=07 EX=naive_barrier               # the broken barrier
python3 tools/interleave/interleave.py tools/interleave/programs/lightswitch.il --outputs
python3 tools/interleave/interleave.py modules/07-semaphores/demo/q_lightswitch_bug.il --why "a w A W b B"
```

The quiz puzzles are in [`demo/`](demo): run them with the explorer after
you have answered on paper.

Then do the [quiz](quiz.md) and check yourself with the
[recall sheet](recall.md). Answers are in
[`answers/07-semaphores/`](../../answers/07-semaphores/), but try first.
