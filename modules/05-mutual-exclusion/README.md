# 05 · Mutual exclusion from scratch ★

| | |
|---|---|
| **Time** | about 4 hours |
| **You should know first** | module 01 (what a race condition is) and module 03 (how to start threads in C) |
| **Lectures** | L05 *The Mutual Exclusion Problem* (slides 1–25); L06 *Basic Constructs* (slides 16–26) |
| **Helps with** | exam questions like "find an order of steps that breaks this", "prove this lock is correct", "can this deadlock?"; module 06 |

**Goal:** build a working lock using nothing but ordinary reads and writes of
shared variables. You will also be able to explain exactly why each simpler
idea fails.

Words in **bold** link to the [glossary](../../GLOSSARY.md) the first time
they appear.

---

## 1. The problem

Two threads share some data. Each thread has a few lines of code that use
that data. We call those lines its [**critical section**](../../GLOSSARY.md#critical-section-cs).
If both threads run their critical sections at the same time, the data can
be corrupted. Module 01 showed this: two threads doing `counter++` can lose
updates.

So we want **[mutual exclusion](../../GLOSSARY.md#mutual-exclusion)**: at
most one thread inside its critical section at any moment.

Here is the challenge for this module. Imagine the computer gives you **no
lock at all**. The only thing that is safe is a single read or a single
write of one variable (L05 slide 5). Can you still build a lock?

Every thread repeats the same four parts:

```c
while (true) {
    non_critical_section();   // normal work; the thread may also stop here forever
    entry_code();             // we design this: "may I go in?"
    critical_section();       // uses the shared data
    exit_code();              // we design this: "I'm out"
}
```

The lecture calls the entry code the *pre-protocol* and the exit code the
*post-protocol*.

## 2. What counts as a correct solution

L05 slides 3–4 list four rules. Exam questions often check them one at a
time, so learn them as a list.

| Rule | Plain meaning |
|---|---|
| **1. Mutual exclusion** | Two threads are never in their critical sections at the same time. |
| **2. No deadlock** | If threads are trying to get in, at least one of them gets in eventually. Nobody stays stuck forever just waiting. |
| **3. No starvation** | Every thread that tries to get in eventually gets in. No thread is left out forever. |
| **4. Works without competition** | If only one thread wants to go in (the other is busy elsewhere, or has stopped), it gets in straight away. |

There is one more rule about stopping. A thread **may** stop forever in its
non-critical section (it simply has no more work). It may **not** stop in
the middle of its entry code, critical section or exit code.

**Two kinds of rules.** Rule 1 is a [**safety**](../../GLOSSARY.md#safety-property)
rule: "something bad never happens". To show it is broken, you give **one
order of steps** that ends with two threads inside together. Rules 2–4 are
[**liveness**](../../GLOSSARY.md#liveness-property) rules: "something good
eventually happens". To show one is broken, you give an order of steps that
can **go on forever** without the good thing happening.

## 3. Building a lock step by step

The lecture builds a correct lock, Dekker's algorithm, through four failed
attempts. Each attempt fixes one problem and creates another. **Knowing why
each attempt fails is the most useful thing in this module for the exam.**

The notation follows the lecture. The threads are called P1 and P2. Each
has a flag: `c1 = 0` means "P1 wants to go in, or is in", and `c1 = 1`
means "P1 is out". (Yes, 0 means "in". It is the opposite of what you might
expect, so read it carefully.)

An **[interleaving](../../GLOSSARY.md#interleaving-schedule)** is one
possible order in which the threads' steps run. To show an attempt fails,
we write out a bad interleaving as a numbered list of steps.

### Attempt 1: take turns

```c
int turn = 1;                         // whose turn is it?
P1:  while (turn == 2) {}   CS   turn = 2;
P2:  while (turn == 1) {}   CS   turn = 1;
```

Each thread waits until it is its turn, enters, then gives the turn away.

| Mutual exclusion | No deadlock | No starvation | Works without competition |
|:--:|:--:|:--:|:--:|
| ✅ | ✅ | ✅ | ❌ |

**Why it fails rule 4.** The threads must strictly alternate: P1, P2, P1,
P2, … Suppose P2 stops in its non-critical section for good. P1 enters
once more and sets `turn = 2`. Now P1 waits for P2 to hand the turn back,
and P2 never will. P1 is stuck forever, even though nobody else is using
the critical section.

**Why mutual exclusion holds** (slide 7). Suppose, to get a contradiction,
that both are inside. Say P1 went in first, at time t1, and P2 went in
later, at time t2, while P1 was still inside.
- When P1 went in, `turn` was 1.
- When P2 went in, `turn` was 2.
- So someone set `turn = 2` between t1 and t2. Only P1's exit code does
  that, but P1 was still inside its critical section.

That is impossible, so both can never be inside together.

You can watch the explorer find the rule-4 problem:
```bash
python3 tools/interleave/interleave.py tools/interleave/programs/attempt1.il
# [FAIL] BLOCKED FOREVER: P2 can never enter because P1 stopped in its non-critical section
```

### Attempt 2: check the other's flag, then raise your own

```c
int c1 = 1, c2 = 1;
P1:  while (c2 != 1) {}   c1 = 0;   CS   c1 = 1;   // wait until P2 is out, then say "I'm in"
```

**Mutual exclusion fails.** Checking and raising are two separate steps,
and the other thread can run in between (slide 11):

| step | P1 does | P2 does | c1 | c2 |
|--:|---|---|:--:|:--:|
| 1 | checks c2: it is 1, so P2 is out | | 1 | 1 |
| 2 | | checks c1: it is 1, so P1 is out | 1 | 1 |
| 3 | sets c1 = 0 | | 0 | 1 |
| 4 | | sets c2 = 0 | 0 | 0 |
| 5 | **enters CS** | **enters CS** | | |

Each thread decided based on information that became out of date before it
acted. This is the same kind of bug as `counter++` in module 01.

### Attempt 3: raise your flag first, then check

```c
P1:  c1 = 0;   while (c2 != 1) {}   CS   c1 = 1;   // say "I want in", then wait until P2 is out
```

**Mutual exclusion now holds.** The argument is the same as for Attempt 1:
when P1 went in, `c2` was 1. For P2 to go in later, it must first set
`c2 = 0` and then see `c1 == 1`. But P1 set `c1 = 0` before going in and
only resets it when it leaves.

**But it can deadlock** (slide 14):
1. P1 sets c1 = 0.
2. P2 sets c2 = 0.
3. P1 sees c2 == 0, so it keeps waiting.
4. P2 sees c1 == 0, so it keeps waiting.

Each is waiting for the other to lower its flag, and neither ever will.
That is a **[deadlock](../../GLOSSARY.md#deadlock)**.

### Attempt 4: if the other also wants in, step back and try again

```c
P1:  c1 = 0;
     while (c2 != 1) { c1 = 1; c1 = 0; }   // lower my flag for a moment, then raise it again
     CS   c1 = 1;
```

Mutual exclusion still holds, but there are two new problems.

- **Livelock** (slide 18). The threads move in perfect step: both raise
  their flags, both see the other's flag, both lower, both raise again, and
  so on forever. Both threads keep running, but neither ever gets in. That
  is a **[livelock](../../GLOSSARY.md#livelock)**. If the timing were even
  slightly different, one thread would get in.
- **Starvation** (slide 17). P2 might always look at P1's flag exactly when
  it is down, so P1 keeps entering again and again while P2 never does.
  That is **[starvation](../../GLOSSARY.md#starvation)** of P2.

> **Deadlock vs livelock:** in a deadlock, nobody *can* move. In a
> livelock, everybody keeps moving but nobody gets anywhere.

### Dekker's algorithm: step back, but take turns doing it

```c
int c1 = 1, c2 = 1, turn = 1;
P1:  c1 = 0;                       // I want in
     while (c2 != 1)               // P2 wants in too
         if (turn == 2) {          // it's P2's turn, so I am the one who steps back
             c1 = 1;               // lower my flag...
             while (turn != 1) {}  // ...and wait until it is my turn
             c1 = 0;               // then raise it again
         }
     CS
     c1 = 1;  turn = 2;            // leave, and give the turn to P2
```

In Attempt 4 **both** threads stepped back, so they could keep copying each
other forever. In Dekker, `turn` decides that only **one** of them steps
back. That removes the livelock. Dekker satisfies all four rules (slide 21).

### Peterson's algorithm: a shorter correct lock

L05 slide 23 writes Peterson's algorithm with `c` flags. L06 slides 23–26
write it with `flag` and `victim`, which is easier to read. **This is the
version to learn by heart:**

```c
// i is my thread number (0 or 1); j = 1 - i is the other thread
void lock(int i) {
    flag[i] = true;                      // (1) "I want to go in"
    victim  = i;                         // (2) "but you can go first"
    while (flag[j] && victim == i) {}    // (3) wait while you want in AND I gave way
}
void unlock(int i) {
    flag[i] = false;                     // "I don't need it any more"
}
```

`victim` simply records who said "you go first" last. That thread is the
one that waits.

L06 explains Peterson as two broken locks joined together:

| Lock | Code | When it breaks | Why |
|---|---|---|---|
| **LockOne** | `flag[i] = true; while (flag[j]) {}` | both threads try at the same moment | both raise their flag, then each waits for the other: deadlock |
| **LockTwo** | `victim = i; while (victim == i) {}` | a thread tries when the other is not interested | it waits for the other to change `victim`, which never happens |
| **Peterson** | both ideas; wait only while **both** conditions hold | never | at the same moment: `victim` lets exactly one in. Alone: `flag[j]` is false, so no waiting |

### Why Peterson is correct (L06 slides 24–26)

**Mutual exclusion.** Suppose both threads are inside. One of them wrote
`victim` last; say it was thread 0, so `victim == 0` from then on. Thread 1
wrote `flag[1] = true` before it wrote `victim`, which was before thread 0
wrote `victim`. So when thread 0 checked its condition, it saw
`flag[1] == true` and `victim == 0`. That means thread 0 must still be
waiting. It cannot be inside. This contradicts our assumption.

**No deadlock.** A thread can only be stuck in the `while` loop, and only
while `victim` equals its own number. `victim` has one value, so at most
one thread can be stuck.

**No starvation.** Thread i waits only while `flag[j]` is true and
`victim == i`. If thread j leaves the critical section, it sets
`flag[j] = false`, and thread i can go in. If thread j tries to come straight
back in, it first sets `victim = j`, and that lets thread i go in. So j can
never overtake i more than once.

The explorer confirms all of this:
```bash
python3 tools/interleave/interleave.py tools/interleave/programs/peterson.il
# [OK] Mutual exclusion   [OK] No deadlock   [OK] No livelock   [OK] No starvation
```

## 4. Common exam mistakes

- **LockOne** deadlocks when both threads try **at the same time**.
  **LockTwo** gets stuck when a thread tries **alone**. Don't mix them up.
- In Peterson, the **order** of `flag[i] = true` and `victim = i` matters.
  Swap them and mutual exclusion breaks (exercise m2 shows how).
- In Attempts 1–4, `c = 0` means "wants to go in". Check which value means
  what before you answer.
- Peterson works for **two** threads only. For more, use the Filter or
  Bakery lock (module 06).
- [**Busy-waiting**](../../GLOSSARY.md#busy-waiting-spinning) (`while (...) {}`)
  keeps the CPU busy while waiting. On one CPU core with priorities, this
  can stop the program completely (module 06).
- "Can it deadlock?" needs an **order of steps** (a numbered list) as the
  answer, not just "yes".

## 5. Careful: what the notes say

- **L05 slides 7, 13, 16, 21:** the tick/cross tables are empty in the
  exported slides. Section 3 above fills them in from the slide text.
- **L01 vs L03:** L01 says "no deadlock" is a *liveness* property, but L03
  lists it under *safety*. Most textbooks call it **liveness**, because it
  promises that something good eventually happens. In the exam, say which
  meaning you use. See [NOTES_ERRATA.md](../../NOTES_ERRATA.md).

## 6. [EXTRA] Why Peterson fails on a real computer, and how to fix it

Everything above assumes that every read and write happens in one single
order, exactly as written in the code. This is called
**[sequential consistency](../../GLOSSARY.md#sequential-consistency)**.
Real computers do not promise this, for speed. Two things can go wrong:

1. **The CPU can delay a write.** When a core writes a variable, the new
   value first goes into a small private queue (a *store buffer*). Other
   cores see it a little later. Meanwhile the core can already go on and
   **read** other variables. So in Peterson:

   ```
   core 0: writes flag[0] = true   (still in core 0's queue, not visible yet)
           reads flag[1]  -> sees false (core 1's write is not visible yet either)
   core 1: writes flag[1] = true   (still in core 1's queue)
           reads flag[0]  -> sees false
   -> both think the other is not interested, and both go in
   ```

2. **The compiler can keep a variable in a register**, so a loop like
   `while (flag[j]) {}` might never read the real variable again.

See it happen on your own machine:

```bash
make run M=05 EX=peterson_plain            # plain variables: thousands of lost updates
make run M=05 EX=peterson_plain ARGS=fence # with a fence: correct
```

**The fix.** A [**memory fence**](../../GLOSSARY.md#memory-fence-barrier-instruction)
forces earlier writes to become visible before any later read. In the
exercises we use C11 *atomic* variables instead. Every read and write of an
atomic variable behaves as if there were fences around it. The helpers
`LOAD(x)` and `STORE(x, v)` in [`common/atomics.h`](../../common/atomics.h)
do this for you, so the code behaves like the lecture model.

In real programs, use the lock your system provides (`pthread_mutex_t`).
These algorithms are for understanding how locks work, not for everyday use.

## 7. Now do

| # | Exercise | Type | What you practise |
|---|---|---|---|
| m1 | `exercises/models/m1_slide24.il` | model | write the L05 slide 24 algorithm for the explorer; answer questions (i)–(iii) |
| m2 | `exercises/models/m2_peterson_swapped.il` | model | see why the order of the two writes in Peterson matters |
| 1 | `exercises/concept/ex1_peterson.c` | core | Peterson's lock in real C |
| 2 | `exercises/concept/ex2_dekker.c` | core | turn Dekker's pseudocode into C |
| 3 | `exercises/concept/ex3_peterson_from_memory.c` | from memory | write Peterson without looking |
| 4 | `exercises/concept/ex4_tournament.c` | challenge | build a 4-thread lock out of 2-thread locks |

```bash
make test M=05                    # test all exercises
make test M=05 T=ex1_peterson     # test one exercise
make run  M=05 EX=ex1_peterson    # run your code on its own
python3 tools/interleave/interleave.py tools/interleave/programs/attempt4.il --step   # you choose which thread runs next
```

Then do the [quiz](quiz.md) and check yourself with the
[recall sheet](recall.md). Answers are in
[`answers/05-mutual-exclusion/`](../../answers/05-mutual-exclusion/), but
try first.
