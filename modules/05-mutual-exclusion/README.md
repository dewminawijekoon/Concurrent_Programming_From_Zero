# 05 · Mutual exclusion from scratch ★

| | |
|---|---|
| **Time** | about 4 hours |
| **Prerequisites** | 01 (races and interleavings), 03 (Pthreads basics) |
| **Lectures** | L05 *The Mutual Exclusion Problem* (slides 1–25); L06 *Basic Constructs* (slides 16–26) |
| **Prepares you for** | exam questions that ask you to *find the bad interleaving*, *prove* mutual exclusion, or say whether an algorithm can deadlock or starve; module 06 (Filter, Bakery) |

By the end you can build a lock out of nothing but reads and writes. You can
also explain, with a concrete schedule, why each "obvious" attempt fails.

---

## 1. The problem (why)

Two threads share a variable. Each wants to run a few lines (its **critical
section**) without the other running *its* critical section at the same time.
Module 01 showed what happens otherwise: `counter++` loses updates.

Here is the catch for this whole module. Assume the hardware gives you **only
atomic loads and stores**: no `lock` instruction, no test-and-set, no OS
mutex (L05 slide 5). Every thread runs the same loop:

```c
while (true) {
    non_critical_section();   // may run forever, or halt here
    pre_protocol();           // <- we design this ...
    critical_section();
    post_protocol();          // <- ... and this
}
```

## 2. The rules a correct solution must obey (what)

These are the rules from L05 slides 3–4. Learn them as a list; the exam
checks each one separately.

| Property | Kind | Meaning |
|---|---|---|
| **Mutual exclusion** | safety | never two threads in their critical sections at once |
| **No deadlock** | liveness | if some threads are trying, *one* of them eventually gets in |
| **No starvation** | liveness | *every* thread that starts trying eventually gets in |
| **Success without contention** | liveness | a thread alone must get in, even if the other halts in its non-critical section |

There is also the rule behind that last row: a thread may halt in its
*non-critical* section, but **never** inside its protocols or critical section.

> **Safety vs liveness:** *safety* = "nothing bad ever happens" (one bad
> state is a counterexample). *Liveness* = "something good eventually
> happens" (a counterexample is an *infinite* run). To disprove safety you
> show a schedule that reaches a bad state. To disprove liveness you show a
> schedule that loops forever without the good thing happening.

## 3. Building the algorithm step by step (how)

L05 builds Dekker's algorithm through four failed attempts. Every attempt
fixes the previous one's bug and introduces a new one. **Knowing exactly why
each attempt fails is the most exam-relevant thing in this module.**

Notation follows the lecture: processes P1 and P2. `c1 = 0` means
"P1 wants in / is in", and `c1 = 1` means "P1 is out".

### Attempt 1: take turns (one shared `turn`)

```c
int turn = 1;
P1: while (turn == 2) {}   CS   turn = 2;
P2: while (turn == 1) {}   CS   turn = 1;
```

| Mutual exclusion | Deadlock | Starvation | Without contention |
|:--:|:--:|:--:|:--:|
| ✅ | ✅ none | ✅ none | ❌ **fails** |

**Why it fails:** it forces **strict alternation**. If P2 halts in its
non-critical section, `turn` stays 2 after P1's next exit, and P1 can never
enter again.

**Why mutual exclusion holds** (proof by contradiction, slide 7). Say P1
entered at t1 and P2 entered at t2 > t1, both still inside. At t1, turn == 1.
At t2, turn == 2. Between t1 and t2 only P1's post-protocol can set turn to 2,
but P1 is still in its critical section. Contradiction.

```bash
python3 tools/interleave/interleave.py tools/interleave/programs/attempt1.il
# [FAIL] BLOCKED FOREVER: P2 can never enter because P1 stopped in its non-critical section
```

### Attempt 2: look at the other's flag, *then* raise yours

```c
int c1 = 1, c2 = 1;
P1: while (c2 != 1) {}   c1 = 0;   CS   c1 = 1;
```

**Mutual exclusion fails.** The check and the set are two separate steps
(slide 11):

| step | P1 | P2 | c1 | c2 |
|--:|---|---|:--:|:--:|
| 1 | sees c2 == 1 | | 1 | 1 |
| 2 | | sees c1 == 1 | 1 | 1 |
| 3 | c1 = 0 | | 0 | 1 |
| 4 | | c2 = 0 | 0 | 0 |
| 5 | **in CS** | **in CS** | | |

This is the same "check-then-act" race as `counter++`: the decision is based
on a value that may change before you act on it.

### Attempt 3: raise your flag first, *then* look

```c
P1: c1 = 0;   while (c2 != 1) {}   CS   c1 = 1;
```

Mutual exclusion now holds (the same contradiction proof: at t1 `c2 == 1`,
at t2 `c1 == 1`, but `c1` stayed 0 because P1 was still inside). It **can
deadlock** instead: P1 sets c1 = 0, P2 sets c2 = 0, and each now waits for the
other forever (slide 14).

### Attempt 4: back off politely

```c
P1: c1 = 0;
    while (c2 != 1) { c1 = 1; c1 = 0; }   // step back, try again
    CS   c1 = 1;
```

Mutual exclusion still holds, but now there are two new failures:

- **Livelock** (slide 18). In perfect lock-step, both raise, both see the
  other, both lower, both raise again … forever. Unlike deadlock, *some*
  schedules succeed; the slightest deviation lets one in.
- **Starvation** (slide 17). P1 can complete a whole cycle (enter, leave,
  re-raise) exactly while P2 has its flag lowered, again and again.

> **Deadlock vs livelock:** in a deadlock nobody *can* move. In a livelock
> everybody keeps moving but nobody makes progress. Both are liveness
> failures.

### Dekker's algorithm: back off, but *take turns* backing off

```c
int c1 = 1, c2 = 1, turn = 1;
P1: c1 = 0;
    while (c2 != 1)
        if (turn == 2) {          // only the thread whose turn it is NOT backs off
            c1 = 1;
            while (turn != 1) {}  // ...and it waits for its turn with its flag DOWN
            c1 = 0;
        }
    CS
    c1 = 1;  turn = 2;            // hand priority to the other
```

`turn` breaks the symmetry that caused Attempt 4's livelock. Dekker satisfies
all four properties (slide 21).

### Peterson's algorithm: the elegant version

L05 slide 23 writes it with c-variables. L06 slides 23–26 write it with
`flag` and `victim`, which is the form to learn:

```c
void lock(int i) {          // i = me, j = 1 - i = the other thread
    flag[i] = true;         // (1) I'm interested
    victim  = i;            // (2) you go first: I'm the victim
    while (flag[j] && victim == i) {}   // (3) wait while you want in AND I'm the victim
}
void unlock(int i) {
    flag[i] = false;        // no longer interested
}
```

L06 presents it as the **combination of two broken locks**:

| Lock | Code | Fails when | Because |
|---|---|---|---|
| **LockOne** | `flag[i] = true; while (flag[j]) {}` | threads run **concurrently** | both raise, both wait: deadlock |
| **LockTwo** | `victim = i; while (victim == i) {}` | a thread runs **alone** (sequentially) | nobody else ever rescues the victim |
| **Peterson** | both, waiting only while *both* conditions hold | never | concurrent: `victim` picks one; alone: `flag[j]` is false |

### Why Peterson works (proof sketches, L06 slides 24–26)

**Mutual exclusion.** Suppose both are inside. Each passed its wait loop, so
for each thread either the other's flag was false, or `victim` was not itself.
Say thread 0 wrote `victim` last, so `victim == 0` from then on. Thread 1 had
already set `flag[1] = true` before writing `victim`. So when thread 0 tested
its condition, `flag[1] == true` and `victim == 0`: thread 0 could not have
passed. Contradiction.

**No deadlock.** A thread can only be stuck in the `while`, and only while it
is the victim. `victim` holds one value, so both cannot be stuck.

**No starvation.** Thread i waits only while `flag[j] && victim == i`. If j
leaves and comes back, j sets `victim = j` on the way in, which releases i.
So j cannot overtake i more than once.

```bash
python3 tools/interleave/interleave.py tools/interleave/programs/peterson.il
# [OK] Mutual exclusion   [OK] No deadlock   [OK] No livelock   [OK] No starvation
```

## 4. Exam traps

- "LockOne deadlocks under **concurrent** execution; LockTwo deadlocks under
  **sequential** execution" is a favourite one-liner. Don't swap them.
- The **order** of `flag[i] = true` and `victim = i` matters (exercise m2).
- The attempts use `c = 0` for *wants in*. That's inverted from intuition, so
  read the slide's convention before answering.
- Peterson is for **two** threads. For n threads use Filter or Bakery
  (module 06).
- Busy-waiting burns CPU while waiting. That costs time on a multicore, and
  on a single core with a priority scheduler it can be fatal (module 06).
- When asked "can it deadlock?", answer with a **schedule** (a numbered list
  of steps), not a feeling.

## 5. Careful: the notes say…

- **L05 slides 7, 13, 16, 21:** the ✓/✗ property tables are blank in the
  exported slides. The tables in section 3 above give the values from the
  slide text.
- **L01 vs L03:** L01 calls "no deadlock" a *liveness* property; L03 lists
  "absence of deadlock" under *safety*. The standard classification (and the
  one used here) is **liveness**: deadlock-freedom promises that something
  good *eventually* happens. See `NOTES_ERRATA.md`.

## 6. [EXTRA] Why your CPU breaks Peterson

Everything above assumes **sequential consistency**: every thread sees all
reads and writes in one global order, as written. Real hardware does not
promise that. On x86, each core puts its stores in a private **store buffer**,
and a later *load* can complete before an earlier *store* becomes visible to
other cores. For Peterson this means:

```
core 0: flag[0] = 1  (sits in core 0's buffer)   reads flag[1] -> 0 (old value!)
core 1: flag[1] = 1  (sits in core 1's buffer)   reads flag[0] -> 0 (old value!)
        -> both enter the critical section
```

The compiler can also keep `flag[j]` in a register and never re-read it. See
it happen:

```bash
make run M=05 EX=peterson_plain            # plain volatile ints: loses thousands of increments
make run M=05 EX=peterson_plain ARGS=fence # with a full memory fence: correct
```

The fix used in the exercises is C11 atomics with the default
(sequentially-consistent) ordering, wrapped as `LOAD`/`STORE` in
[`common/atomics.h`](../../common/atomics.h). Real code uses the lock your
OS gives you (`pthread_mutex_t`). These algorithms are for understanding,
not production.

## 7. Now do

| # | Exercise | Track | What it trains |
|---|---|---|---|
| m1 | `exercises/models/m1_slide24.il` | concept | model the L05 slide 24 exercise; answer (i)–(iii) |
| m2 | `exercises/models/m2_peterson_swapped.il` | concept | why the order of the two writes matters |
| 1 | `exercises/concept/ex1_peterson.c` | core | Peterson in real C with atomics |
| 2 | `exercises/concept/ex2_dekker.c` | core | translate Dekker from the slide |
| 3 | `exercises/concept/ex3_peterson_from_memory.c` | from memory | write Peterson from a blank page |
| 4 | `exercises/concept/ex4_tournament.c` | challenge | build a 4-thread lock from 2-thread locks |

```bash
make test M=05                    # all exercises
make test M=05 T=ex1_peterson     # one exercise
make run  M=05 EX=ex1_peterson    # run your code on its own
python3 tools/interleave/interleave.py tools/interleave/programs/attempt4.il --step   # be the scheduler
```

Then work through [`quiz.md`](quiz.md) and check yourself against
[`recall.md`](recall.md). Answers are in
[`answers/05-mutual-exclusion/`](../../answers/05-mutual-exclusion/); try
first.
