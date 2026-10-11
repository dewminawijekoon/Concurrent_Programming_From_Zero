# 06 · N-thread locks and the hardware ★

| | |
|---|---|
| **Time** | about 5 hours |
| **You should know first** | module 05 (the four rules for a lock, Peterson's lock, `LOAD`/`STORE`) |
| **Lectures** | L06 *Basic Constructs* (slides 27–40: Filter, bounded waiting, Bakery); L08-TLP *Thread-Level Parallelism* (slides 8–15, 22, 24–25, 31–33: caches and hardware synchronization); L09 *Basic Constructs II* (slide 3: problems with busy waiting) |
| **Helps with** | exam questions like "a test-and-set lock on one core with priorities: what goes wrong?", "what if test-and-set is not atomic?", "why are Bakery ties broken by thread id?", "what if `max()` is not atomic?"; module 07 (semaphores replace spinning with sleeping); module 15 |

**Goal:** build locks for any number of threads, first in software only
(Filter, Bakery), then with one special hardware instruction (test-and-set,
compare-and-swap). Understand, in plain words, what the hardware's caches do
to a spinning thread, and why spinning can freeze a one-core machine.

Words in **bold** link to the [glossary](../../GLOSSARY.md) the first time
they appear, or are explained right where they appear.

---

## 1. The problem

Peterson's lock (module 05) works for exactly **two** threads. Real
programs have 4, 8 or 100 threads. We need locks for *n* threads.

There are two ways to get there:

1. **Software only.** Keep the module 05 rule: the only safe operations are
   a single read or a single write of one shared variable. The Filter lock
   and the Bakery lock (L06) work like this.
2. **Ask the hardware for help.** Every modern CPU has instructions that
   read a variable *and* write it in one [**atomic**](../../GLOSSARY.md#atomic)
   step. With those, a lock needs just one shared variable (L08-TLP).

The second way is what real locks use. But it brings a new problem: the
CPU's caches. A badly written spin lock can make every core in the machine
slower. And on a machine with one core and thread priorities, a spinning
thread can stop the program completely (L09 slide 3).

## 2. The Filter lock: Peterson for n threads (L06 slides 27–33)

### The idea

Picture a building with n − 1 **waiting rooms** stacked on top of each
other, called **levels** 1, 2, …, n − 1. The critical section is at the top.
A thread climbs one level at a time. At each level, **at least one thread
must stay behind** if several are trying to get in. So:

- at most n − 1 threads can get past level 1,
- at most n − 2 can get past level 2,
- …
- at most 1 can get past level n − 1, into the critical section.

At each level, the thread that **arrived last** is the one that may have to
stay. It is called the **victim** of that level, exactly as in Peterson.

### The code

```c
int level[n];    // level[i] = the level thread i is trying to reach (0 = not trying)
int victim[n];   // victim[L] = the thread that arrived at level L last

void lock(int i) {
    for (int L = 1; L < n; L++) {
        level[i]  = L;        // "I want to enter level L"
        victim[L] = i;        // "but anyone else may go first"
        while (someone_else_at_level_L_or_higher(i, L) && victim[L] == i) {}
    }
}
void unlock(int i) {
    level[i] = 0;             // back to the ground floor
}
```

The waiting condition has the same shape as Peterson's:
**wait while (someone else is at my level or higher) AND (I am the victim
here).** For n = 2 there is one level, and Filter *is* Peterson.

### A trace with three threads

Threads A, B and C all call `lock()` at the same time. There are 2 levels.

| step | what happens | level (A, B, C) | victim[1] | victim[2] |
|--:|---|:--:|:--:|:--:|
| 1 | A, B and C write `level = 1` | 1, 1, 1 | | |
| 2 | A writes `victim[1] = A`, then B, then C (C is last) | 1, 1, 1 | C | |
| 3 | A checks: victim[1] is C, not A, so A may climb | | | |
| 4 | B checks: victim[1] is C, so B may climb too | | | |
| 5 | C checks: others are at level ≥ 1 and victim[1] is C, so **C waits at level 1** | | | |
| 6 | A and B write `level = 2`; A writes `victim[2] = A`, then B writes `victim[2] = B` | 2, 2, 1 | C | B |
| 7 | A checks level 2: victim[2] is B, so **A enters the critical section** | | | |
| 8 | B checks: A is at level 2 and victim[2] is B, so **B waits at level 2** | | | |
| 9 | A leaves: `level[A] = 0`. Now nobody else is at level ≥ 2, so B enters | 0, 2, 1 | C | B |

At level 1 one thread (C) stayed behind. At level 2 another (B) stayed
behind. Only one reached the top.

### Why at most n − L threads get past level L (slide 32)

Suppose too many threads, n − L + 1 of them, got past level L. Look at the
one that wrote `victim[L]` **last**; call it X. All the others wrote
`level = L` before they wrote `victim[L]`, and that was before X wrote
`victim[L]`. So when X checked its condition, it saw those threads at level
L or higher, and it saw `victim[L] == X`. Both parts of the condition were
true, so X had to wait. X could not have got past. That contradicts our
assumption. The last thread to arrive always stays behind.

With L = n − 1 this says: at most **1** thread is in the critical section.
That is mutual exclusion.

### No starvation, but no fairness

Each level behaves like a Peterson lock, so a waiting thread is never
stuck forever: as soon as another thread arrives at its level (and becomes
the victim), or everyone above it leaves, it can climb. The Filter lock is
[**starvation**](../../GLOSSARY.md#starvation)-free (slide 33).

But it is **not fair** in the everyday sense. A thread can be **overtaken**
by a thread that arrived after it. The explorer shows one way:

```bash
python3 tools/interleave/interleave.py modules/06-locks-and-hardware/demo/filter_overtake.il --why "0 1 CS 1 CS 0"
# IS possible: P0 arrives first, but P1 writes victim[1] before P0 does,
# so P0 becomes the victim and P1 goes first.
```

Run the full check on the repo's Filter program too:
`python3 tools/interleave/interleave.py tools/interleave/programs/filter3.il`
(all five checks are OK).

## 3. Fairness: the doorway and bounded waiting (L06 slide 34)

Starvation-free only promises "**eventually**". It says nothing about how
long you wait or how many threads go ahead of you. To talk about fairness we
split `lock()` into two parts:

- The **doorway**: the first part of `lock()`. A thread always gets through
  it in a fixed number of its own steps. It never waits for anyone here.
  *Example:* taking a ticket from the machine at a bakery counter.
- The **waiting part**: the rest, where the thread may have to wait.
  *Example:* standing there until your number is called.

Now we can say what "fair" means:

| Property | Plain meaning |
|---|---|
| **Bounded waiting** (*r*-bounded) | once A has finished its doorway, any other thread can go into the critical section ahead of A at most *r* times |
| **First-come-first-served** | if A finishes its doorway **before B starts** its doorway, then A enters before B. (This is bounded waiting with *r* = 0.) |

"Before B starts" matters. If the two doorways overlap in time, either
thread may go first.

## 4. Lamport's Bakery lock (L06 slides 35–40)

### The idea

Like a bakery with a ticket machine. When you arrive, you take a number one
higher than every number you can see. Then you wait until everyone with a
smaller number has been served.

### The code

```c
bool flag[n];    // flag[i] = true: thread i wants to enter, or is inside
int  label[n];   // label[i] = thread i's ticket number

void lock(int i) {
    // doorway
    flag[i]  = true;                                   // "I'm interested"
    label[i] = max(label[0], ..., label[n-1]) + 1;     // take the next ticket
    // waiting part
    while (there is some k != i with flag[k] && (label[k], k) < (label[i], i)) {}
}
void unlock(int i) {
    flag[i] = false;                                   // "no longer interested"
}
```

**Comparing pairs.** `(label[k], k) < (label[i], i)` means: k's label is
smaller, **or** the labels are equal and k's id is smaller (slide 35). This
is called *lexicographic* order: the same rule as sorting words, where you
compare the first letter, and look at the second only if the first ones are
the same. *Example:* (3, 2) < (4, 0) because 3 < 4. (3, 0) < (3, 2) because
the labels tie and 0 < 2.

### A trace with three threads

| step | what happens | flag (A, B, C) | label (A, B, C) |
|--:|---|:--:|:--:|
| 1 | A: flag = true, sees labels 0, 0, 0, takes label 1 | t, f, f | 1, 0, 0 |
| 2 | A: nobody else has a raised flag, so **A enters** | | |
| 3 | B: flag = true, sees the biggest label 1, takes 2 | t, t, f | 1, 2, 0 |
| 4 | C: flag = true, sees the biggest label 2, takes 3 | t, t, t | 1, 2, 3 |
| 5 | B waits for A (smaller label). C waits for A and B | | |
| 6 | A leaves: flag[A] = false. **B enters** (C still sees B's (2, B) < (3, C)) | f, t, t | 1, 2, 3 |
| 7 | B leaves. **C enters** | f, f, t | 1, 2, 3 |

The threads entered in the order they took their tickets.

### Why ties are broken by thread id

The doorway reads the labels one at a time, so `max()` is **not** one
atomic step. Two threads can read the labels at the same moment, see the
same maximum, and take the **same** label. Then the labels alone cannot
decide who goes first. We need a tie-breaker that is different for every
thread: the thread id. The explorer shows what happens without it
(`modules/06-locks-and-hardware/demo/bakery_ties.il`):

| Waiting condition used | Result (explorer, 2 threads) |
|---|---|
| `(label[k], k) < (label[i], i)` (the real one) | correct |
| `label[k] < label[i]` (labels only) | **mutual exclusion violated**: both have label 1, neither is smaller, both go in |
| `label[k] <= label[i]` (labels only, "be polite") | **deadlock**: both have label 1, each waits for the other |

### Why it works (slide 40)

- **No deadlock.** Among the threads that are waiting, one has the smallest
  pair (label, id). Pairs are never equal, because ids differ. That thread
  waits for nobody, so it goes in.
- **First-come-first-served.** If A finished its doorway before B started
  its doorway, then B's `max()` read A's label, which was already written.
  So B's label is bigger, and B must wait while A's flag is up.
- **Mutual exclusion.** Suppose A and B are both inside, and A has the
  smaller pair. B got in, so when B last checked A, it saw one of two
  things:
  - *a pair for A bigger than B's own.* But each thread's labels only
    grow, so A's current pair would be even bigger. That contradicts "A
    has the smaller pair". So this case cannot happen.
  - *`flag[A] == false`.* A is inside now, so A raised its flag **after**
    B's check. B had written its label before checking. So A's doorway
    read B's label and took a bigger one. Again A's pair would be bigger:
    contradiction.

  Both cases are impossible, so A and B cannot both be inside.
- **No starvation.** First-come-first-served means nobody who arrives later
  can go ahead of you, and only threads already in front of you can. There
  are at most n − 1 of those, and each goes in once.

### What if `max()` is not atomic? (slide 37)

The lock is **still correct**. Our C exercise reads the labels one by one,
and the explorer checks the same thing for 2 threads
(`tools/interleave/programs/bakery2.il --bound 4`) and for 3 threads
(`modules/06-locks-and-hardware/demo/bakery3.il --bound 4`). The only
effect of a non-atomic `max()` is that two threads can get the same label,
and the id breaks that tie.

The **flag** is what makes this safe. A thread raises its flag *before* it
starts reading labels. So another thread can see "this one is in its
doorway, its label may be about to change". Model exercise m3 removes the
flag ("a label of 0 means not interested") and the explorer finds two
threads inside together: one thread checked the other while the other was
half-way through taking its label.

### Labels grow without bound

Labels are never reset (slide 39: "labels are always increasing"). After
millions of lock calls they become huge numbers, and a fixed-size integer
would eventually overflow and wrap around to a small number. That would
break first-come-first-served and even mutual exclusion. In practice a
64-bit counter will not overflow in your lifetime, but it is a real
weakness of the algorithm. The explorer needs `--bound 4` for Bakery for
the same reason: otherwise it would have infinitely many states.

### Filter vs Bakery

| | Filter | Bakery |
|---|---|---|
| Shared variables | `level[n]`, `victim[n]` | `flag[n]`, `label[n]` |
| Mutual exclusion | yes | yes |
| No deadlock, no starvation | yes | yes |
| First-come-first-served | **no**: a thread can be overtaken | **yes** |
| Weak point | slow: n − 1 levels, each reading n variables | labels grow forever |

Both read up to n shared variables just to get in. That is slow, and it is
the main reason real locks use hardware instructions instead.

## 5. What the hardware gives us (L08-TLP slides 31–33)

### Atomic read-modify-write instructions

A **read-modify-write** instruction reads a memory location, computes a
new value and writes it back, all as one step. No other core can touch that
location in between. L08-TLP slide 31 lists these:

| Instruction | What it does, in one step | In `common/atomics.h` |
|---|---|---|
| **Test-and-set** | write `true` (1) into x and return the **old** value | `TEST_AND_SET(x)` |
| **Atomic exchange** (swap) | write a given value into x and return the **old** value | `EXCHANGE(x, v)` |
| **Fetch-and-increment** | add 1 to x and return the **old** value | `FETCH_ADD(x, 1)` |
| **Compare-and-swap** (CAS) | if x equals `expected`, write `new` and return true; otherwise change nothing and return false | `CAS(x, expected, new)` |
| **Load-linked / store-conditional** (LL/SC) | a pair: *load-linked* reads x and starts watching it; *store-conditional* writes x **only if nobody wrote x since the load-linked**, and says whether it worked | (no direct C version; on ARM and RISC-V the C compiler builds `CAS` from it) |

Test-and-set is just exchange with the value `true`. LL/SC is how some CPUs
(ARM, RISC-V, the MIPS used in the slides) provide the same power as CAS:
you read, compute, and try to write; if someone got in between, the write
fails and you try again.

**CAS is the most flexible.** With a retry loop it can build any of the
others (exercise 5):

```c
long fetch_and_increment(x):
    loop:
        old = LOAD(x)
        if CAS(x, old, old + 1): return old    // nobody changed x since we read it
        // someone did: read again and retry
```

### The test-and-set (TAS) spin lock

```c
bool held = false;
void lock()   { while (TEST_AND_SET(held)) {} }   // old value true: someone has it, try again
void unlock() { held = false; }
```

A **spin lock** is a lock where waiting threads loop ("spin"), checking
again and again: this is [**busy-waiting**](../../GLOSSARY.md#busy-waiting-spinning).
Why this lock works: test-and-set reads and writes in **one** step. If two
threads call it at the same moment, the hardware does one call fully, then
the other. The first one gets the old value `false` and has the lock. The
second gets `true` and keeps trying. The explorer agrees
(`tools/interleave/programs/tas_lock.il`): mutual exclusion and no deadlock
are OK.

It is **not starvation-free**: the explorer finds a run where T1 grabs the
lock again and again and T0 never wins. Nothing in the lock remembers who
has been waiting longest.

### What if test-and-set is not atomic? (exam question)

Suppose "test-and-set" were really two steps: `temp = lock;` then
`lock = 1;`. Two things go wrong
(`tools/interleave/programs/tas_nonatomic.il`):

1. **Mutual exclusion breaks.** T0 reads 0. T1 reads 0. Both write 1.
   Both saw "free", so both enter.
2. **Everyone can get stuck.** T1 reads 1 (T0 holds the lock). T0 unlocks:
   `lock = 0`. Now T1 does its write: `lock = 1`. The lock says "taken" but
   nobody holds it. Every thread spins forever. The explorer reports this as
   a livelock: threads keep running, nobody gets in.

See both on your own machine:
```bash
make run M=06 EX=tas_nonatomic                 # usually gets stuck (problem 2)
make run M=06 EX=tas_nonatomic ARGS=checkfirst # write only if free: lost updates (problem 1)
make run M=06 EX=tas_nonatomic ARGS=atomic     # the real instruction: correct
```

## 6. Caches, in plain words (L08-TLP slides 8–15, 22, 24–25)

To understand why some spin locks are slow, you need a picture of the
memory system.

### Caches and cache lines

Main memory is slow: one read can take as long as a hundred simple
instructions. So every core keeps a **cache**: a small, fast, private memory
holding copies of the data it used recently. Caches do not copy single
variables. They copy fixed-size blocks called **cache lines**, usually 64
bytes. *Example:* reading one `int` brings it and its 15 neighbours into
the cache.

### The coherence problem

If two cores each have a copy of variable `x`, and core 1 writes `x = 5`,
core 2's copy is now **out of date** (slide 8). **Cache coherence** is the
hardware's job of hiding this. A coherent memory promises (slide 10):

1. a read returns the most recently written value, and
2. all cores see the writes to one variable in the **same order**.

(Slide 11 adds **consistency**, a separate question: in what order do
writes to *different* variables become visible? That is what broke plain
Peterson in module 05, section 6.)

### How the hardware keeps copies in agreement

Each cache line in each cache is marked with a state. The simple version:

| State | Meaning |
|---|---|
| **Shared** | this cache has a read-only copy; other caches may have copies too |
| **Exclusive / modified** | this cache has the **only** copy and may write it; memory may be out of date |
| **Invalid** | this copy is out of date; reading it means fetching it again |

**Reading** is cheap if you already have a shared copy: no one else is
disturbed. **Writing** needs the exclusive state, so the other copies must
be dealt with first. There are two ways (slides 13–14):

| Write-invalidate (most common) | Write-update (write-broadcast) |
|---|---|
| tell every other cache "throw your copy away" | send the new value to every other cache |
| the next read by another core misses and fetches the line again | other cores keep reading their own copy |
| little traffic if one core writes many times in a row | uses much more bandwidth: every write is broadcast |

And there are two ways to find out who has a copy (slides 12, 24–25):

| Snooping | Directory |
|---|---|
| all caches sit on one shared bus and **listen** to every request ("snoop") | one place (the directory) **records** which caches have a copy of each line |
| simple and fast for a few cores | the directory sends messages only to the caches that have a copy |
| the bus becomes a bottleneck with many cores | scales to many cores and to machines where memory is spread out (NUMA) |

### Why TAS spinning is slow, and TTAS is better

Every test-and-set is a **write**, even when it fails (it writes `true` over
`true`). So each try by a waiting core:

1. takes the lock's cache line in exclusive state,
2. invalidates the copies in all other caches,
3. and the next waiting core does the same thing back.

With 8 waiting cores, the line bounces between caches nonstop. All that
traffic also slows down the holder, who has to get the line back to unlock.

**Test-and-test-and-set (TTAS)** waits with plain reads first:

```c
void lock() {
    for (;;) {
        while (held) {}                       // "test": read only, from my own cache
        if (!TEST_AND_SET(held)) return;      // looks free: now really try
    }
}
```

While the lock is held, each waiter reads its **own shared copy**: no
traffic at all. Only when the holder writes `held = false` do the copies get
invalidated; the waiters re-read, see "free", and only then try the
expensive test-and-set. This is exactly the second spin lock on L08-TLP
slide 32 ("if coherence: load, branch, then exchange"). Slide 33 gives the
reason: the spin reads come from the cache.

Measure it yourself: `make run M=06 EX=ex4_spinlocks`. On modern CPUs the
difference can be smaller than the theory suggests, because the hardware
has gotten smarter, but the reason is worth knowing.

### False sharing (slide 22)

Slide 22 splits the extra cache misses caused by coherence into two kinds:

- **True sharing**: two cores really use the same variable. Unavoidable if
  they must communicate.
- **False sharing**: two cores use **different** variables that happen to
  sit in the **same cache line**. Each write by one core invalidates the
  other core's copy of the whole line, even though they never touch each
  other's data.

*Example:* 4 threads each count in their own `long counter[4]`. The four
counters take 32 bytes, so they all sit in one cache line. Every increment
steals the line from the other three cores. The fix is **padding**: put each
counter in its own cache line. Exercise 6 measures this; on our 16-core test
machine the padded version was about 10 times faster with 4 threads.

## 7. Busy waiting vs blocking, and the priority problem (L09 slide 3)

**Busy waiting** (spinning) keeps the CPU busy doing nothing useful. On a
machine with spare cores and short critical sections that is fine, even
fast: the waiter notices the release immediately. **Blocking** means the
waiting thread asks the operating system to put it to sleep, and to wake it
up when the lock is free. A sleeping thread uses no CPU. Semaphores
(module 07) and `pthread_mutex_t` block.

### The priority problem (exam question type)

L09 slide 3: thread H has high priority, thread L has low priority, one
CPU core. The scheduler always runs the highest-priority thread that is
**ready** (able to run).

1. L takes a test-and-set spin lock and enters its critical section.
2. H becomes ready (for example, the data it was waiting for arrives).
   H has higher priority, so the scheduler stops L and runs H.
3. H tries to take the lock. It is held, so H spins.
4. H is spinning, which means it is **running**, so it is still ready.
   It still has the highest priority. The scheduler never runs L again.
5. L never reaches `unlock()`. H spins forever. Nobody makes progress.

The explorer checks this exact story (model exercise m1). With
`--priority H=2,L=1` it reports a livelock. Without priorities (several
cores, or a fair scheduler) the same code works.

The repo's program `tools/interleave/programs/tas_priority.il` is the
exam-style version, with two high-priority threads A and B and one low one, C:

```bash
python3 tools/interleave/interleave.py tools/interleave/programs/tas_priority.il --priority A=2,B=2,C=1 --no-halt
# LIVELOCK: A and B take turns spinning forever; C holds the lock but never runs
```

**The fix** (slide 3: "sleep and wakeup"): use a **blocking** lock. When H
finds the lock taken, it goes to sleep, so it is no longer ready. Now the
only ready thread is L. L runs, unlocks, and that wakes H. Model exercise m2
checks this: no livelock, no deadlock.

**Exam answer pattern** (for "what goes wrong?"): name the thread that
holds the lock, say why it can never run (a higher-priority thread is
always ready because spinning counts as running), say why the spinner can
never succeed (the lock is only released by the thread that cannot run),
and name the result (the system makes no progress: a livelock). Then give
the fix (blocking, or [EXTRA] priority inheritance, section 10).

## 8. API cheat-sheet: `common/atomics.h`

| Macro | Meaning |
|---|---|
| `shared_int`, `shared_bool`, `shared_long` | shared variables that behave like the lecture's (atomic) |
| `LOAD(x)`, `STORE(x, v)` | read / write, in program order for all threads |
| `TEST_AND_SET(x)` | set `x = true`, return the old value |
| `EXCHANGE(x, v)` | set `x = v`, return the old value |
| `FETCH_ADD(x, d)` | add `d` to `x`, return the old value |
| `CAS(x, expected, new)` | if `x == expected`, set `x = new` and return true; else return false |
| `cpu_relax()` | a hint to the CPU: "I am spinning" (saves power, helps the other thread on the same core) |
| `spin_pause(&spins)` (in `nlock.h`) | `cpu_relax()`, and every 64 calls also give the CPU away |

## 9. Common exam mistakes

- Filter has **n − 1** levels, not n. "At most n − L threads get past level
  L", so at level n − 1 at most one: mutual exclusion.
- Filter is starvation-free but **not** first-come-first-served. Bakery is
  both.
- First-come-first-served is about the **doorway**: A must *finish* its
  doorway before B *starts* its doorway. If the doorways overlap, either
  may go first.
- In Bakery, two threads **can** have the same label. That is why the pair
  (label, id) is compared. "Ties are impossible" (slide 40) is about
  *pairs*, not labels.
- A non-atomic `max()` does **not** break Bakery. Removing the flag does.
- Test-and-set returns the **old** value. "Old value false" means "it was
  free and now I have it".
- The single-core priority problem is a **livelock** (H keeps running), not
  a deadlock (nobody can run). Both mean "no progress", but say livelock and
  explain why.
- A TAS lock is deadlock-free but **not** starvation-free.
- TTAS is not "more correct" than TAS. Both are correct. TTAS creates less
  cache traffic while waiting.
- False sharing involves **different** variables in the same cache line,
  not the same variable.

## 10. Careful: what the notes say

- **L06 slides 29–31:** the symbol "there exists" (∃) was lost in the export
  and shows as `$`. `while (($ k != i) (level[k] >= L && victim[L] == i))`
  means "while there exists some k other than i with level[k] ≥ L, and
  victim[L] == i". Slides 30–31 also have unbalanced brackets. Read it as
  the code in section 2.
- **L06 slides 29–31** call the release method `unlock()` on one slide and
  `release(int i)` on the next. Same thing.
- **L06 slide 28:** the constructor's loop starts at `i = 1`, so it never
  sets `level[0]`. It is harmless (Java fills new arrays with 0), but in C
  you must set every element yourself.
- **L06 slides 37–38:** the Bakery waiting condition is written without
  `k != i`. That is fine: for k = i the pair comparison is "(label[i], i)
  > (label[i], i)", which is false.
- **L06 slide 40:** an arrow was lost: "If DA  DB then A's label is
  smaller" means "if A's doorway finishes before B's doorway starts, then
  A's label is smaller". "Labels are strictly increasing" means that each
  thread's successive labels keep increasing. "Ties are impossible (why?)"
  is about (label, id) pairs; labels themselves can tie.
- **L06 slide 40 notes** ask "How about the starvation?" with no answer.
  Answer: first-come-first-served implies starvation-freedom (section 4).
- **L08-TLP slide 31** says test-and-set "sets under condition". In this
  course's exam and in most books, test-and-set means "set to 1 and return
  the old value". The lock built from it is the same either way.
- **L08-TLP slide 32** shows spin locks in MIPS assembly. In plain words:
  the first loops on an atomic exchange (a TAS lock); the second first
  loads the lock and branches back while it is non-zero, and only then
  does the exchange (a TTAS lock).
- **L09 slide 3** describes the high/low priority problem without naming
  it. Some books (Tanenbaum) call exactly this the *priority inversion
  problem*. Others use "priority inversion" for a three-thread version with
  a blocking lock (section 11). In the exam, describe what happens rather
  than relying on the name.
- **L08-TLP** is headed "CS5312 High Performance Computer Architecture". It
  is still part of this course (background for this module). See
  [NOTES_ERRATA.md](../../NOTES_ERRATA.md).

## 11. [EXTRA] Beyond the slides

**Priority inversion with a blocking lock.** Blocking fixes the two-thread
story, but add a medium-priority thread M that does not need the lock. L
holds the lock, H sleeps waiting for it, and M (ready, higher than L) runs
for a long time. L cannot run, so it cannot release the lock, so H waits
for M, a *lower*-priority thread. The usual fix is **priority inheritance**:
while L holds a lock that H wants, L runs with H's priority. (This bug
famously froze the Mars Pathfinder lander in 1997.)
```bash
python3 tools/interleave/interleave.py modules/06-locks-and-hardware/demo/priority_inversion.il --priority H=3,M=2,L=1 --no-halt
```

**The ticket lock: Bakery with fetch-and-increment.** Bakery's doorway
needs n reads because it has to find the maximum. One fetch-and-increment
does the same job in one step:
```c
shared_long next_ticket = 0, now_serving = 0;
void lock()   { long my = FETCH_ADD(next_ticket, 1); while (LOAD(now_serving) != my) cpu_relax(); }
void unlock() { STORE(now_serving, LOAD(now_serving) + 1); }   // only the holder writes it
```
It is first-come-first-served, needs two variables instead of 2n, and has
no ties at all. Linux used this design for years.

**Back-off.** When a TTAS waiter sees the lock free but loses the race, it
can wait a random, growing amount of time before trying again. Fewer cores
then try at once, so there is less traffic.

**Why real code uses `pthread_mutex_t`.** It spins briefly (fast when the
wait is short), then blocks (no wasted CPU when the wait is long), and it
deals with priorities on systems that support them.

## 12. Now do

| # | Exercise | Type | What you practise |
|---|---|---|---|
| 1 | `exercises/concept/ex1_filter.c` | core | the Filter lock in C, tested with 2, 4 and 8 threads |
| 2 | `exercises/concept/ex2_bakery.c` | core | the Bakery lock; the test checks first-come-first-served |
| 3 | `exercises/concept/ex3_bakery_from_memory.c` | from memory | write Bakery without looking |
| 4 | `exercises/concept/ex4_spinlocks.c` | core | TAS and TTAS spin locks, plus a benchmark table |
| 5 | `exercises/concept/ex5_cas.c` | core | fetch-and-increment and atomic max built from CAS |
| 6 | `exercises/concept/ex6_false_sharing.c` | warm-up | pad counters onto their own cache lines; measure the speed-up |
| m1 | `exercises/models/m1_priority_spin.il` | model | the exam's TAS-lock-with-priorities question |
| m2 | `exercises/models/m2_priority_block.il` | model | the same story with a blocking lock |
| m3 | `exercises/models/m3_bakery_no_flag.il` | model | why Bakery needs its flag |

```bash
make test M=06                         # test all exercises
make test M=06 T=ex2_bakery            # test one exercise
make run  M=06 EX=ex4_spinlocks        # TAS vs TTAS vs pthread mutex table
make run  M=06 EX=ex6_false_sharing    # packed vs padded counters table
make run  M=06 EX=tas_nonatomic        # break_it: test-and-set done in two steps
python3 tools/interleave/interleave.py tools/interleave/programs/tas_priority.il --priority A=2,B=2,C=1 --no-halt
```

Then do the [quiz](quiz.md) and check yourself with the
[recall sheet](recall.md). Answers are in
[`answers/06-locks-and-hardware/`](../../answers/06-locks-and-hardware/),
but try first.
