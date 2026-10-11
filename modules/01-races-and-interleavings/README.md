# 01 · Races and interleavings ★

| | |
|---|---|
| **Time** | about 4 hours |
| **You should know first** | module 00 (what a thread is; how to start one) |
| **Lectures** | L01 *Introduction* (slides 17–28, 33–34); L03 *Concurrent Programming Abstraction* (slides 2–5, 14–16, 32–44) |
| **Helps with** | Lab 01 Ex 2 (observe the race) and Ex 3 (fix it with a mutex); exam questions like "is this output possible?", "list the possible final values", "how many interleavings?"; modules 05–09 |

**Goal:** explain exactly *why* `counter++` in two threads can lose updates,
list every result a small concurrent program can produce, count its
interleavings, and fix a race in C with a lock.

Words in **bold** link to the [glossary](../../GLOSSARY.md) the first time
they appear.

---

## 1. The problem

Four threads each add 1 to a shared counter 100,000 times. The answer
should be 400,000. Here is what one run of Lab 01 Ex 2 printed on a
16-core laptop (from `make run M=01 EX=lab1_observe_race ANS=1`):

| run | result | lost |
|--:|--:|--:|
| 1 | 180,973 | 219,027 |
| 2 | 150,197 | 249,803 |
| 3 | 226,625 | 173,375 |

More than half of the additions disappeared, and every run gives a
different answer. Nothing crashed. No error was printed. The program is
simply wrong, and wrong by a different amount each time.

The same thing happens in L01's prime example (slides 17–22). Ten threads
share a counter that hands out "the next number to test". If two threads
can get the same number, some numbers are tested twice and the counter
falls behind.

This module explains why this happens and how to reason about it.

## 2. The idea: a few words we need

| Word | Plain meaning | Tiny example |
|---|---|---|
| [**Atomic**](../../GLOSSARY.md#atomic) step | a step that happens all at once; no other thread can see it half-done | reading one `int` |
| [**Interleaving**](../../GLOSSARY.md#interleaving-schedule) | one possible order in which the threads' atomic steps run, keeping each thread's own steps in order | A1 B1 A2 B2 |
| [**Race condition**](../../GLOSSARY.md#race-condition-data-race) | the result depends on the interleaving, and some interleavings give a wrong result (L03 slides 2, 40) | two `counter++` give 1 instead of 2 |
| **Lost update** | one thread's write overwrites another thread's write, so one update vanishes (L03 slide 40) | both read 5, both write 6 |
| **Check-then-act** | a thread checks a condition, then acts on it, but the condition changes in between | two threads both see "1 ticket left" and both buy it |
| [**Critical section**](../../GLOSSARY.md#critical-section-cs) | the lines of code that use shared data and must not run in two threads at once (L03 slide 3) | read the counter, add 1, write it back |

## 3. The mechanism

### 3.1 `counter++` is three steps, not one (L03 slides 37–40)

In C, `counter++` looks like one action. But most processors work on
*registers* (tiny storage cells inside the core), not directly on memory.
This is called a **load-store** design. So the processor really does:

```
load  counter -> R      (copy the value from memory into register R)
add   1 to R
store R -> counter      (copy R back to memory)
```

Each of the three is atomic. The whole sequence is not. Another thread can
run between any two of them. Here is the interleaving that loses an update,
with two threads and `counter` starting at 0:

| step | thread 1 | thread 2 | R1 | R2 | counter |
|--:|---|---|:--:|:--:|:--:|
| 1 | load counter → R1 | | 0 | | 0 |
| 2 | | load counter → R2 | 0 | 0 | 0 |
| 3 | add 1 to R1 | | 1 | 0 | 0 |
| 4 | | add 1 to R2 | 1 | 1 | 0 |
| 5 | store R1 → counter | | 1 | 1 | **1** |
| 6 | | store R2 → counter | 1 | 1 | **1** |

Both threads added 1, but the counter is 1, not 2. Thread 2's store
overwrote thread 1's store with the same old value plus one. Thread 1's
update is lost.

**Why the result is never higher than expected.** Each store writes "some
value I read, plus one". A thread can only *forget* other threads' updates,
never invent new ones. So the result is always at most N × M.

**"What if the machine has an INC instruction?"** L03 slide 38 notes that if
`n = n + 1` became a single increment instruction, every interleaving would
give 2. On one core that is true, because the core cannot switch threads in
the middle of an instruction. On a **multicore** machine, even a single
`inc` instruction is not atomic unless the processor is told to lock the
memory for it. In C you get that with an atomic operation such as
`atomic_fetch_add` (section 3.5).

You can watch the explorer find the bad order. It tries every interleaving:

```bash
python3 tools/interleave/interleave.py tools/interleave/programs/counter_race.il --outputs
# Possible outputs ...:  1 1   1 2   2 2      ("1 1" means one update was lost)
python3 tools/interleave/interleave.py tools/interleave/programs/counter_race.il --why "1 1"
```

### 3.2 The shared counter in the prime example (L01 slides 17–28)

**The task** (slide 17): print every prime from 1 to 10¹⁰ using 10
processors, and get close to a 10-fold speedup.

**First try: static split** (slide 18). Give each thread a fixed block:
thread 0 tests 1 to 10⁹, thread 1 tests the next 10⁹, and so on. Module 00,
exercise 2 did this. **The problem** (slide 19): the blocks are not equal
work. Big numbers take longer to test, and the threads with big numbers
finish last. The others sit idle.

**Second try: dynamic split with a shared counter** (slides 20–22). Every
thread loops: "take the next number from the counter, test it". A thread
that finishes early simply takes more numbers. This is **dynamic load
balancing**.

**The catch** (slides 23–24): `getAndIncrement()` is really

```
temp  = value;        // read
value = temp + 1;     // write
return temp;
```

Slide 24 shows the bad case with two threads, Blue and Red. Blue reads 1.
Before Blue writes, Red runs `getAndIncrement` twice: it reads 1 and writes
2, then reads 2 and writes 3. Now Blue writes its 2. The counter has gone
**back** from 3 to 2. Number 1 was handed out twice, and number 2 will be
handed out again too.

**The fix** (slides 26–28): make the read and the write **one indivisible
step**. Two ways:

- **Mutual exclusion** (slide 28): put both lines inside a lock, so only
  one thread at a time can be between the read and the write. Java writes
  this as a `synchronized` block. In C you use `pthread_mutex_t`.
- **A hardware read-modify-write instruction** (slide 27): the processor
  reads, changes and writes a memory word as one step. In C:
  `atomic_fetch_add(&value, 1)`.

Exercise 3 builds the dynamic version with a mutex-protected counter.

### 3.3 Interleavings: the model we reason with (L03 slides 14–16, 32–34)

We cannot predict the timing of threads (module 00: asynchrony). So L03
uses a simple model:

- Each thread is a sequence of **atomic steps**.
- A run of the program is **one interleaving** of all the threads' steps.
  The steps of one thread stay in order. Steps of different threads can mix
  in any way.
- It does not matter whether the threads run on one core (taking turns) or
  many. We always reason with interleavings (slide 15).
- The interleaving must be **fair**: no thread is ignored forever (slide 32).
  See [fair scheduler](../../GLOSSARY.md#fair-scheduler).
- A concurrent program is correct only if it is correct for **every** fair
  interleaving (slide 32).

**Example** (slide 33). P1 does `p1a; p1b`. P2 does `p2a; p2b`. The
interleavings are:

```
p1a p1b p2a p2b     p1a p2a p1b p2b     p1a p2a p2b p1b
p2a p2b p1a p1b     p2a p1a p2b p1b     p2a p1a p1b p2b
```

Six orders. Notice that `p1a` always comes before `p1b`.

**Counting them** (slide 34). With n threads, each doing m atomic steps:

```
number of interleavings = (n·m)! / (m!)^n
```

Why: there are n·m time slots in total. If all the steps were different and
could go anywhere, there would be (n·m)! orders. But each thread's own m
steps must stay in their written order. Of the m! ways to arrange one
thread's steps, only one is allowed. So we divide by m! once for each
thread.

Worked example: 3 threads with 2 steps each.

```
(3·2)! / (2!)^3 = 720 / 8 = 90
```

A second way to see it: thread 1 picks 2 of the 6 slots (15 ways), thread 2
picks 2 of the 4 slots left (6 ways), and thread 3 takes the last 2 (1 way).
15 × 6 × 1 = 90.

The number explodes fast. Two threads with 10 steps each already have
184,756 orders. Ten threads with 10 steps each have about 2.4 × 10⁹².
**This is why testing cannot show a concurrent program is correct**
(slide 32): you can never try every order. Instead we reason about all of
them at once, or check tiny models with the explorer.

```bash
make run M=01 EX=interleavings ARGS="3 2"      # the worked solution above
make run M=01 EX=interleavings ARGS="lens 1 2 3"  # threads with different lengths
```

### 3.4 Which steps are atomic changes the answer (L03 slides 35–41)

**Example 1** (slide 35). `c1 = 2, c2 = 3`. P1 does `c1 = c1 * c2`. P2 does
`c1 = c1 + c2`. If each line is one atomic step, there are two orders:

- P1 then P2: c1 = 2 × 3 = 6, then 6 + 3 = **9**.
- P2 then P1: c1 = 2 + 3 = 5, then 5 × 3 = **15**.

But on a load-store machine each line is "load c1, compute, store c1". Now
both threads can load the old value 2 before either stores. The last store
wins: **6** (P1 stores last) or **5** (P2 stores last). So the possible
results become 5, 6, 9 and 15. (Quiz question 13 asks you to check this
with the explorer.)

**Example 2** (slide 41, the lecture gives no answer). `y = 0, z = 0`.
P1 does `x = y + z`. P2 does `y = 1; z = 2`. On a load-store machine, P1
really does:

```
P1:  load y -> a;   load z -> b;   x = a + b       P2:  y = 1;   z = 2
```

| Final x | An order that gives it |
|:--:|---|
| 0 | P1 loads y and z before P2 starts |
| 1 | P2 sets y = 1; P1 loads y (1) and z (0); P2 sets z = 2 |
| 2 | P1 loads y (0); P2 sets y = 1 and z = 2; P1 loads z (2) |
| 3 | P2 finishes first; P1 loads y (1) and z (2) |

So **x can be 0, 1, 2 or 3**. Model exercise m2 checks this with the
explorer.

Look at the row for 2: it is only possible because P1 loads `y` **before**
`z`. If the compiler loaded `z` first, seeing z = 2 would mean P2 had
already set y = 1, so P1 would get 3. Then only 0, 1 and 3 are possible.
**The answer depends on exactly which steps are atomic and in what order**
(slide 37). In an exam, state your assumption.

### 3.5 Fixing a race

The general fix is to turn the critical section into something that
behaves like **one atomic step** for the other threads.

| Fix | How | Cost | When to use |
|---|---|---|---|
| **Mutex** (lock) | `pthread_mutex_lock(&m); ... ; pthread_mutex_unlock(&m);` around the critical section | threads wait for each other; each lock/unlock takes time | any critical section, short or long |
| **Atomic instruction** | `atomic_fetch_add(&counter, 1)` | cheaper than a lock, but only for one simple update | a single counter or flag |
| **Do not share** | each thread works on its own local variable, and combines the result once at the end (with one lock) | almost free | sums, counts, anything you can combine at the end |

Lab 01 Ex 3 compares the first and the last. Our measurement
(`make run M=01 EX=lab2_mutex_vs_local`), 100,000 increments per thread:

| threads | global mutex: locks taken | local sum: locks taken | global mutex is slower by |
|--:|--:|--:|--:|
| 1 | 100,000 | 1 | about 65× |
| 4 | 400,000 | 4 | about 150× |
| 16 | 1,600,000 | 16 | about 440× |

The global mutex is slow for two reasons. Each lock and unlock costs time,
even with one thread. And with many threads, they queue up for the same
lock: only one can hold it, and the rest wait. This is called **lock
contention**. The local version almost never waits. (With `-O2` the
compiler even turns the private loop into a single addition, which is only
safe because no other thread can see that variable.)

**Check-then-act needs the same fix.** In exercise 2, `withdraw()` checks
"is there enough money?" and then subtracts. If two threads both pass the
check before either subtracts, the balance goes negative. The check and the
action must be inside **the same** critical section. Locking them
separately does not help, because another thread can still run between
them.

### 3.6 Finding races: three tools

1. **Make the bad timing likely.** The race window between a read and a
   write is a few nanoseconds wide. A program can pass a thousand runs and
   still be wrong. In the exercises, `race_window()` calls `sched_yield()`
   right inside the window. That asks the operating system to run another
   thread now, so the bug shows up almost every time.
2. **ThreadSanitizer.** It watches every memory access while the program
   runs. It reports two threads touching the same variable, at least one of
   them writing, with nothing ordering them. It reports this even when the
   result happened to be right:

   ```bash
   make tsan M=01 T=ex2_lost_update     # before your fix: "WARNING: ThreadSanitizer: data race"
   ```

   It names both lines of code involved. After your fix it stays quiet.
3. **The interleaving explorer.** For a small model, it tries every order
   and gives a certain answer, with the shortest bad order (section 3.1).

## 4. Why the fixes work

**Why a lock makes `counter++` correct.** Call the three steps of one
increment "load, add, store". With the lock, a thread does all three while
holding the lock, and only one thread can hold it at a time. So between
one thread's load and its store, no other thread can load or store the
counter. The increments therefore happen one after another, never
overlapped. Each increment loads the value the previous one stored. After
N × M increments the counter is exactly N × M.

**Why the local sum is correct.** Each local variable is touched by one
thread only, so no interleaving can mix two threads' updates of it. The one
shared update per thread (`counter += mine`) is inside the lock, so by the
argument above, none of those N updates is lost.

**Why the result is always at most N × M without a lock** (section 3.1):
every store writes one more than some value that was in memory earlier, so
an update can be lost but never invented.

## 5. Safety and liveness (L01 slides 33–34, L03 slides 42–44)

| Kind | Plain meaning | How you disprove it | Examples |
|---|---|---|---|
| [**Safety**](../../GLOSSARY.md#safety-property) | something bad **never** happens | show **one** finite run that reaches the bad state | "the counter is never wrong", mutual exclusion |
| [**Liveness**](../../GLOSSARY.md#liveness-property) | something good **eventually** happens | show a run that goes on **forever** without the good thing | "every thread that wants the lock eventually gets it", no [starvation](../../GLOSSARY.md#starvation) |

A lost update breaks a safety property: the final counter is wrong, and one
bad interleaving is enough to show it.

L03 (slides 3–5) also states the critical-section problem the way
operating-systems books do, with three conditions: **mutual exclusion**,
**progress** (if nobody is inside and some threads want in, one of them
gets in, without unlimited delay) and **bounded waiting** (after a thread
asks to enter, other threads can go in only a limited number of times
before it does). Module 05 uses an equivalent list of four rules.

## 6. API cheat-sheet

| Call | What it does |
|---|---|
| `pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;` | a mutex, ready to use |
| `pthread_mutex_init(&m, NULL)` | set up a mutex at run time (for one inside a struct) |
| `pthread_mutex_lock(&m)` | wait until nobody holds `m`, then hold it |
| `pthread_mutex_unlock(&m)` | release `m`, so one waiting thread can take it |
| `atomic_fetch_add(&x, 1)` | add 1 to an `atomic_long x` as one step; returns the old value (C11, `<stdatomic.h>`) |
| `sched_yield()` | "let another thread run now" (used to widen race windows in tests) |
| `interleave.py F --outputs` | every output the model can print |
| `interleave.py F --why "1 1"` | is this output possible? If yes, show one order |
| `interleave.py F --infinite "A B"` | can it print `A B A B ...` forever? |

## 7. Common exam mistakes

- Saying `counter++` is atomic because it is "one line". Write it as load,
  add, store, and show the interleaving.
- Forgetting that a lost update can make the result much lower than
  expected, but **never higher**.
- Listing only the "nice" results (9 and 15) for the slide 35 program on a
  load-store machine. With separate loads and stores, 5 and 6 are possible
  too.
- Counting interleavings with (n·m)! alone. You must divide by (m!)ⁿ,
  because each thread's own steps keep their order.
- Protecting the check and the action with **two separate** lock/unlock
  pairs. They must be one critical section.
- "I ran it 1000 times and it was fine, so it is correct." A test can only
  show a bug, never prove there is none (L03 slide 32).
- Forgetting to unlock on every path out of a function (for example an
  early `return` inside the critical section). The next `lock()` waits
  forever (Lab 01 Q3.3; try `make run M=01 EX=lab2_mutex_vs_local ARGS=forget`).

## 8. Careful: what the notes say

- **L01 vs L03 on safety and liveness.** L01 (slide 34) calls "no deadlock"
  a *liveness* property. L03 (slide 43) lists "absence of deadlock" under
  *safety*. Most textbooks call it liveness, because it promises that
  something good eventually happens (some thread gets in). There is a sense
  in which L03 is also right: "the system never reaches a state where every
  thread is blocked" can be disproved with one finite run that ends in
  that state. In an exam, say which definition you use.
  See [NOTES_ERRATA.md](../../NOTES_ERRATA.md).
- **L03 slide 41** asks for the possible values of x but gives no answer.
  Section 3.4 derives them: 0, 1, 2, 3 (assuming y is loaded before z).
- **L03 slide 15** says "without loss of generality" we model execution as
  interleaved. In plain words: we lose nothing by pretending the steps
  happen one at a time in some order, because two steps that really happen
  at the same instant on two cores give the same result as one of the two
  orders.
- **L01 slide 21** loops `while (j < 10¹⁰) { j = counter.getAndIncrement(); ... }`.
  The test uses the *previous* `j`, so each thread tests one number beyond
  the limit at the end. Exercise 3 checks the new number before testing it.
  (Minor, but exam answers should get it right.)
- **L01 slides 17–18** print "1010" and "109": these mean 10¹⁰ and 10⁹.

## 9. [EXTRA] More than the slides

- **In C, a data race is undefined behaviour.** If two threads access the
  same ordinary variable at the same time and one of them writes, the C
  standard says nothing at all about what the program does. The compiler
  may assume it never happens and optimise accordingly. Use a mutex or
  atomic variables. `volatile` does **not** fix races: it only stops the
  compiler from caching a variable. (Lab track 1 uses `volatile` only to
  keep the compiler from merging the loop into one addition, so the race is
  visible, as it would be with the lab's `-O0`.)
- **How low can it go?** Two threads each do `n++` twice. The explorer
  shows the final value can be as low as **2**, not just 3
  (`answers/01-races-and-interleavings/puzzles/p2_two_increments.il`). With
  more increments per thread, the minimum is still 2. The trick: one thread
  reads early, the other does almost all its work, then the first thread
  overwrites it.
- **Real hardware is even stranger.** Quiz puzzle 14 is impossible in the
  lecture's model. On a real multicore CPU it *can* happen, because a core
  may make its write visible to other cores late. Module 05, section 6
  explains this, and why atomics and locks prevent it.

## 10. Now do

| # | Exercise | Type | What you practise |
|---|---|---|---|
| m1 | `exercises/models/m1_get_and_increment.il` | model | write L01's `getAndIncrement` as separate steps; see the explorer find the duplicate number |
| m2 | `exercises/models/m2_x_equals_y_plus_z.il` | model | the L03 slide 41 program on a load-store machine: prove x ∈ {0, 1, 2, 3} |
| 1 | `exercises/concept/ex1_count_interleavings.c` | warm-up | list interleavings; count them with the formula (big numbers, overflow) |
| 2 | `exercises/concept/ex2_lost_update.c` | core | see a lost update and a check-then-act race fail every time; fix them with a mutex |
| 3 | `exercises/concept/ex3_shared_counter_primes.c` | core | L01's dynamic load balancing with a correct shared counter |
| L1 | `exercises/lab/lab1_observe_race.c` | lab | Lab 01 Ex 2: run the race, collect min / max / range / error % for the report |
| L2 | `exercises/lab/lab2_mutex_vs_local.c` | lab | Lab 01 Ex 3: global mutex vs local sum, with the timing table |

```bash
make test M=01                                   # test all exercises
make test M=01 T=ex2_lost_update                 # test one
make tsan M=01 T=ex2_lost_update                 # ThreadSanitizer: see the race reported
make run  M=01 EX=lab1_observe_race              # tables for Lab 01 Q2.1 and Q2.3
make run  M=01 EX=lab2_mutex_vs_local            # table for Lab 01 Q3.1
make run  M=01 EX=interleavings ARGS="3 2"       # interleavings calculator
python3 tools/interleave/interleave.py modules/01-races-and-interleavings/exercises/models/m2_x_equals_y_plus_z.il --outputs
```

Then do the [quiz](quiz.md) and check yourself with the
[recall sheet](recall.md). Answers are in
[`answers/01-races-and-interleavings/`](../../answers/01-races-and-interleavings/),
including the explorer files for the quiz puzzles, but try first.
