# 10 · Deadlocks ★

| | |
|---|---|
| **Time** | about 5 hours |
| **You should know first** | module 03 (pthread mutexes), module 05 (what a deadlock is in a lock algorithm), module 07 (semaphores) helps |
| **Lectures** | L11 *Deadlocks* (all slides; based on Tanenbaum, *Modern Operating Systems*); L06 *Basic Constructs* (slides 7–8: the four conditions, slide 14: the two-lock exercise) |
| **Helps with** | exam questions like "given these threads and their code positions, write the allocation and still-needed matrices, find a schedule that deadlocks and one that completes"; "T/F: preventing hold-and-wait is always appropriate"; Banker's algorithm by hand; modules 09 and 15 |

**Goal:** know exactly when a deadlock can happen (the four conditions),
how to spot one (graphs and matrices), and the four ways to deal with it.
Run the Banker's algorithm and the detection algorithm by hand and in code,
and write multi-lock code that cannot deadlock.

Words in **bold** link to the [glossary](../../GLOSSARY.md) the first time
they appear, or are explained right where they appear.

---

## 1. The problem

Two threads each need two locks:

```c
thread 1:  lock(L1);  lock(L2);  ...work...  unlock(L2);  unlock(L1);
thread 2:  lock(L2);  lock(L1);  ...work...  unlock(L1);  unlock(L2);
```

1. Thread 1 locks L1.
2. Thread 2 locks L2.
3. Thread 1 tries to lock L2: it is taken, so thread 1 waits.
4. Thread 2 tries to lock L1: it is taken, so thread 2 waits.

Each thread waits for something only the other can give, so both wait
forever. This is a [**deadlock**](../../GLOSSARY.md#deadlock). It is the
second case of the exercise on L06 slide 14. See it happen:

```bash
make run M=10 EX=two_locks_deadlock              # stops after a few rounds
make run M=10 EX=two_locks_deadlock ARGS=ordered # both lock L1 first: always finishes
```

Nothing crashes and nothing is printed. The program just stops. That is
what makes deadlocks nasty: in testing, the bad timing may never happen,
and then it happens on a customer's machine.

## 2. Resources and the definition

The lecture talks about **processes** and **resources**; everything works
the same for threads and locks.

A **resource** is anything a process needs exclusive use of for a while: a
printer, a tape drive, a lock, a database record (L11 slide 2). Using one
takes three steps: **request** it, **use** it, **release** it (slide 3). If
the request cannot be granted, the process waits.

Two kinds (slide 4):

| Kind | Meaning | Example |
|---|---|---|
| **Preemptable** | can be taken away from its owner and given back later, with no harm | memory (the system can save a page to disk and bring it back), the CPU |
| **Non-preemptable** | taking it away would make the owner fail | a printer half-way through a page, a lock in the middle of an update |

Deadlocks are about non-preemptable resources. With preemptable ones the
system can simply take the resource back.

**Definition** (slide 5): a set of processes is deadlocked if **each**
process in the set is waiting for an event that **only another process in
the set** can cause. Usually the event is "release a resource". None of
them can run, none can release anything, and none can be woken up.

## 3. The four conditions (L11 slide 6, L06 slide 8)

A deadlock can happen only if **all four** hold at the same time:

| Condition | Plain meaning | In the two-lock example |
|---|---|---|
| **1. Mutual exclusion** | each resource is held by at most one process at a time | a mutex has one owner |
| **2. Hold and wait** | a process that holds resources may ask for more, and keeps what it has while waiting | thread 1 keeps L1 while waiting for L2 |
| **3. No preemption** | a resource cannot be taken away; only its owner releases it | nobody can force thread 2 to give up L2 |
| **4. Circular wait** | there is a cycle of two or more processes, each waiting for a resource held by the next one | thread 1 waits for thread 2, which waits for thread 1 |

They are **necessary** conditions: if you make sure even **one** of them
can never hold, deadlock is impossible. That is the idea behind
prevention (section 8).

## 4. Pictures: resource-allocation graphs (L11 slides 7, 10)

Draw processes as **circles** and resources as **squares**. Arrows:

- resource → process: "this resource is **held by** this process"
- process → resource: "this process is **waiting for** this resource"

```
   (T1) ----waits for----> [L2]
    ^                        |
    |                        |
  held by                 held by
    |                        |
   [L1] <---waits for---- (T2)
```

With **one instance of each resource**, a deadlock exists **exactly when
the graph has a cycle**. The processes on the cycle are deadlocked.

A bigger example (our own): A holds R and wants S; B holds S and wants T;
C holds T and wants R; D holds U and wants S; E holds V and wants U.

```
  [R] -> (A) -> [S] -> (B) -> [T] -> (C) -> [R]      the cycle: A, B, C
                 ^
                 |
  [U] -> (D) ----+                                   D waits for S (held by B)
   ^
   |
  (E) <- [V]                                         E waits for U (held by D)
```

A, B and C are on the cycle: they are deadlocked. D and E are not on the
cycle, but they wait for processes that will never release anything, so
they are stuck forever too. Exercise 1 finds cycles with a depth-first
search.

**Several instances.** If a resource type has several identical copies (3
printers), a cycle no longer proves a deadlock: another copy might be
released by a process outside the cycle. Then we need the matrices of
section 6.

## 5. Four strategies (L11 slide 8)

| Strategy | Idea | Cost |
|---|---|---|
| **1. Ignore it** (the ostrich algorithm) | pretend deadlocks don't happen | cheap; fine if deadlocks are very rare and prevention is expensive |
| **2. Detect and recover** | let deadlocks happen, notice them, then fix them | needs a detection algorithm and a way to recover |
| **3. Avoid** (dynamic avoidance) | before granting each request, check that it cannot lead to deadlock | needs to know each process's maximum needs in advance |
| **4. Prevent** | design the system so one of the four conditions can never hold | restricts how programs may use resources |

**The ostrich algorithm** (slide 9): stick your head in the sand. UNIX and
Windows mostly do this for general resources: a rare hang costs less than
restricting every program. It is a trade-off between **convenience** and
**correctness**.

## 6. Detection and recovery (L11 slides 10–13)

### One instance of each resource

Build the resource-allocation graph and look for a cycle (section 4,
exercise 1).

### Several instances of each resource: the matrices

The slides show these only as pictures, so here they are in full. With
*m* resource types and *n* processes:

| Name | Shape | Meaning |
|---|---|---|
| **E** (existing) | vector of m numbers | how many instances of each resource exist in total |
| **A** (available) | vector of m numbers | how many are free right now |
| **C** (current allocation) | n × m matrix | row i = what process i holds now |
| **R** (requests) | n × m matrix | row i = what process i is waiting for now |

They always satisfy: for each resource, **what is held plus what is free
equals what exists** (E = column sums of C + A).

**The detection algorithm** asks: "could every process still finish, if
each one got what it is waiting for?"

1. Find a process that is not marked yet and whose **whole request row
   fits** into A (every entry of its R row ≤ the matching entry of A).
2. If you find one: assume it gets its request, runs to the end and
   releases **everything it holds**. Add its C row to A. Mark it. Go back
   to step 1.
3. If there is none: stop. **Every unmarked process is deadlocked.**

**Worked example** (our own numbers, `calc/examples/detect.txt`). Three
resource types, E = (4 4 3):

| process | C (holds) | R (waits for) |
|---|---|---|
| P1 | 2 1 0 | 0 2 0 |
| P2 | 1 2 1 | 1 0 0 |
| P3 | 0 1 1 | 0 0 1 |
| P4 | 1 0 0 | 1 0 1 |

Column sums of C = (4 4 2), so A = (4 4 3) − (4 4 2) = **(0 0 1)**.

- P1 wants (0 2 0): needs 2 of R2, 0 are free. No.
- P2 wants (1 0 0): no R1 free. No.
- P3 wants (0 0 1): fits! P3 finishes and releases (0 1 1). A = (0 1 2).
- Now P1 still needs 2 of R2 (1 free), P2 and P4 each need an R1 (0 free).
  Nobody fits.

**Result: P1, P2 and P4 are deadlocked.** Check it:
```bash
make run M=10 EX=banker ARGS="detect modules/10-deadlocks/calc/examples/detect.txt"
make run M=10 EX=banker ARGS="detect modules/10-deadlocks/calc/examples/detect_ok.txt"  # one more R1: no deadlock
```

**When to run it?** On every request (catches deadlocks at once, but
costs time), every few minutes, or when the CPU is mostly idle (a sign
that many processes are stuck).

### Recovery (slide 13)

| Method | How | Problem |
|---|---|---|
| **Preemption** | take a resource away from one process and give it to another | only works for some resources; often needs a human |
| **Rollback** | save each process's state regularly (a *checkpoint*); on deadlock, restart one process from an earlier checkpoint, freeing what it got since | needs checkpoints; work since then is lost |
| **Killing processes** | kill a process in the cycle; the others get its resources | lost work; choose one that can safely be rerun from the start (not one halfway through updating a database) |

## 7. Avoidance: safe states and the Banker's algorithm (L11 slides 14–18)

### Safe and unsafe states

Avoidance needs every process to declare its **maximum** need in advance.
A state is **safe** if there is **some order** in which every process can
get its full maximum and finish, even if all of them ask for their maximum
right now. Otherwise it is **unsafe**.

- Safe means the system can always avoid deadlock (by granting in that order).
- Unsafe means deadlock is **possible**, not certain. A process might ask for
  less than its maximum, or release something early. But the system can no
  longer guarantee that everyone finishes.

The **Banker's algorithm** grants a request only if the state after
granting is still safe. A bank works the same way: it never lends money if
it could then be unable to pay out all its customers' credit lines.

### One resource type (slide 17)

10 units in total (our own numbers, `calc/examples/one_resource.txt`):

| process | has | max | still needs |
|---|---|---|---|
| A | 3 | 9 | 6 |
| B | 2 | 4 | 2 |
| C | 2 | 7 | 5 |

Free = 10 − 7 = **3**. Is it safe?

1. Only B's need (2) fits in 3. B finishes and returns its 2: free = 5.
2. C's need (5) fits. C finishes: free = 7.
3. A's need (6) fits. A finishes: free = 10.

**Safe**, with the order B, C, A.

Now **A asks for 1 more unit**. Pretend to grant it: A has 4 (needs 5),
free = 2. B fits (needs 2): free = 4. Now A needs 5 and C needs 5, but only
4 are free. **Unsafe**, so the Banker says: A must **wait**, even though a
free unit exists. (The same request from B would be granted: try it.)

### Several resource types (slide 18)

The same idea with vectors. **Need = Max − Allocation**, row by row.

**Safety algorithm:**

1. Let **work** = Available. Nobody is finished.
2. Find an unfinished process whose **Need row ≤ work** (in every column).
3. If found: it can finish; **work += its Allocation row**; mark it
   finished; add it to the safe sequence; go to 2.
4. If none is found: **safe** if everybody is finished, else **unsafe**.

**Worked example** (our own numbers, `calc/examples/safe.txt`). E = (10 5 7):

| process | Allocation | Max | Need = Max − Alloc |
|---|---|---|---|
| P1 | 1 1 0 | 5 2 2 | 4 1 2 |
| P2 | 2 0 1 | 3 2 3 | 1 2 2 |
| P3 | 0 2 2 | 5 3 3 | 5 1 1 |
| P4 | 3 0 1 | 6 1 2 | 3 1 1 |
| P5 | 1 1 1 | 2 4 2 | 1 3 1 |

Allocation column sums = (7 4 5), so Available = (10 5 7) − (7 4 5) =
**(3 1 2)**.

| step | process that fits | its Need | ≤ work? | work after it returns its Allocation |
|--:|---|---|---|---|
| start | | | | (3 1 2) |
| 1 | P4 | (3 1 1) | yes | (3 1 2) + (3 0 1) = (6 1 3) |
| 2 | P1 | (4 1 2) | yes | + (1 1 0) = (7 2 3) |
| 3 | P2 | (1 2 2) | yes | + (2 0 1) = (9 2 4) |
| 4 | P3 | (5 1 1) | yes | + (0 2 2) = (9 4 6) |
| 5 | P5 | (1 3 1) | yes | + (1 1 1) = (10 5 7) |

**Safe; safe sequence P4, P1, P2, P3, P5.** (There may be others. At the
end, work always equals E: everything has been given back.)

**The request step.** When process p asks for a vector `req`:

1. If `req > Need[p]` in some column: error, p broke its promise.
2. If `req > Available` in some column: p must wait (not enough free).
3. Pretend to grant: Available −= req, Allocation[p] += req.
4. Run the safety algorithm. Safe: grant for real. Unsafe: undo, p waits.

In the example, **P4 asking for (1 0 0)** is granted (P4 then needs
(2 1 1) and still finishes first). **P5 asking for (0 1 0)** must wait:
afterwards Available = (3 0 2) and every Need row needs at least one R2.

```bash
make run M=10 EX=banker ARGS="safe modules/10-deadlocks/calc/examples/safe.txt"
make run M=10 EX=banker ARGS="safe modules/10-deadlocks/calc/examples/unsafe.txt"     # one less R1: unsafe
make run M=10 EX=banker ARGS="request modules/10-deadlocks/calc/examples/safe.txt P5 0 1 0"
```

### Resource trajectories (slide 16)

The slide draws two processes as a 2D picture: one axis is process A's
progress, the other is process B's. A path through the picture is one
schedule. Regions where both would hold the same resource are forbidden
(shaded). An **unsafe** region is a corner that the path can enter but not
leave without hitting a forbidden region: once inside, deadlock cannot be
avoided. The Banker's algorithm keeps the path out of such corners.

### Why avoidance is rarely used

Processes seldom know their maximum needs in advance, the number of
processes changes all the time, and resources can disappear (a printer
breaks). So avoidance is mostly a tool for reasoning, and an exam favourite.

## 8. Prevention: attack one condition (L11 slides 19–24)

| Condition attacked | How | Why it is hard / when it fails |
|---|---|---|
| **Mutual exclusion** | don't give processes the resource directly. *Spooling*: only one printer *daemon* (a background helper program) talks to the printer; others put their jobs in a queue on disk. Principle: assign a resource only when really needed, to as few processes as possible. | not every resource can be spooled (a lock, a database record); the queue on disk can itself fill up |
| **Hold and wait** | request **everything at the start**, all at once; or before asking for anything new, **release everything** and ask again for all of it | a process may not know at the start what it will need; resources sit idle while held "just in case"; a process may starve waiting for everything to be free at once |
| **No preemption** | take resources away by force | usually not viable: imagine taking the printer away half-way through a job (slide 22) |
| **Circular wait** | **number all resources** and request them only in **increasing order** (to get Rj you must not hold any Ri with i ≥ j) | a global order must be agreed on and followed everywhere; sometimes the natural order of the code is different, so code must take locks earlier than needed |

**Why ordering works.** Suppose there were a cycle of waiting. Each process
in the cycle holds some resource and waits for a higher-numbered one (that
is the only kind of waiting allowed). Going round the cycle, the numbers go
up at every step, yet you end where you started. That is impossible, so
there is no cycle. In practice, **lock ordering is the most used
prevention technique in multithreaded code**: exercises 5 and 6.

**Exam T/F: "Preventing hold-and-wait is not always suitable." True.** A
process often doesn't know all its needs at the start (it depends on the
input), and holding everything from the start keeps resources idle that
others could use, which lowers concurrency. It can also starve a process
that needs many resources at once.

## 9. The exam question: threads with code positions (multiple instances)

Typical question: several threads each acquire some instances of R1 and R2
at numbered lines. "At this point, write the allocation and still-needed
matrices. Give a schedule that deadlocks, and one that completes."

**Example** (our own numbers, `calc/examples/threads.txt`). R1 has 3
instances, R2 has 2. An `acquire` waits until **all** the instances it
asks for are free, then takes them in one step.

```
T1:  1) acquire(R1, 2)   2) acquire(R2, 1)   3) release all
T2:  1) acquire(R2, 1)   2) acquire(R1, 2)   3) release all
T3:  1) acquire(R2, 1)   2) acquire(R1, 2)   3) release all
```

**Step 1. Max claim** (the most each thread ever holds): T1 (2 1), T2 (2 1),
T3 (2 1).

**Step 2. The state after each thread has done its line 1:**

| thread | Allocation (R1 R2) | Still needed (R1 R2) |
|---|---|---|
| T1 | 2 0 | 0 1 |
| T2 | 0 1 | 2 0 |
| T3 | 0 1 | 2 0 |

Available = (3 2) − (2 2) = **(1 0)**. T1 needs an R2 (none free). T2 and
T3 each need 2 R1 (one free). **Nobody can move: this is a deadlock**, and
the schedule "T1 line 1, T2 line 1, T3 line 1" leads to it.

**Step 3. A schedule that completes:** run the threads one after another:
T1 lines 1–3, then T2 lines 1–3, then T3 lines 1–3. Each finds everything
free.

**How to find these by hand.** For a deadlock, give each thread its first
resources so that what is left is too little for *every* thread's next
request. For a completing schedule, the safe sequence of the Banker's
algorithm (from the start state) is one.

Check any version of the question with the calculator:
```bash
make run M=10 EX=banker ARGS="schedule modules/10-deadlocks/calc/examples/threads.txt"
make run M=10 EX=banker ARGS="schedule modules/10-deadlocks/calc/examples/threads.txt at T1=1 T2=1 T3=1"
```
It prints the matrices, runs the safety check, and searches **every**
order of steps for a deadlocking schedule and a completing one. Write your
own question in the same format (the format is at the top of
`calc/banker.c`).

## 10. Common exam mistakes

- "Unsafe" does **not** mean "deadlocked". It means the system can no
  longer guarantee that everyone finishes.
- **Need = Max − Allocation**, not Max − Available.
- In both algorithms, when a process finishes you add back its
  **Allocation** (what it held), not its Need.
- Detection uses the **request** matrix R (what processes are waiting for
  now). Banker's uses **Need** (the most they may still ask for).
- With several instances per resource, a cycle in the graph is **not**
  proof of deadlock.
- The four conditions are **necessary**, not sufficient: all four can hold
  without a deadlock happening yet (circular wait is what actually makes it
  happen).
- Lock ordering prevents **circular wait**, not hold-and-wait: threads still
  hold one lock while waiting for the next.
- Preemptable resources (memory, CPU) do not cause deadlocks in the
  classic sense: the system can take them back.
- A deadlock answer needs a **schedule** (numbered steps) and the final
  state ("T1 holds …, waits for …"), not just "yes, it can deadlock".

## 11. Careful: what the notes say

- **L11 slides 10–12, 14–18:** the detection example, the matrices E, A, C,
  R, the Banker's tables and the trajectory picture are **images only**.
  Sections 6–7 above explain the algorithms in full, with our own
  numbers. See [NOTES_ERRATA.md](../../NOTES_ERRATA.md).
- **L11 slides 24–25:** the summary table of prevention approaches is an
  image and the last slide is empty. Section 8 has the table.
- **L06 slide 8 vs L11 slide 6:** the mutual exclusion condition is
  phrased slightly differently ("assigned to 1 process" vs "assigned to 1
  process or is available"). Same meaning.
- **L11 slide 23** asks "Is this generally feasible?" about resource
  ordering without answering. Answer: it works and is widely used for
  locks, but a global order is hard to agree on in a big system, and code
  sometimes must take resources earlier than it needs them.
- **L11 slide 9** says UNIX and Windows ignore deadlocks. That is about
  general resources; inside the kernel, developers do use lock ordering.

## 12. [EXTRA] Beyond the slides

- **`pthread_mutex_trylock`** gives a way to break hold-and-wait at run
  time: lock the first mutex, *try* the second; if it fails, release the
  first, wait a little, and start again. This avoids deadlock but can
  livelock if every thread keeps backing off in step (compare module 05,
  Attempt 4), so add a random delay.
- **Lock ordering by address.** When objects have no natural id, programs
  sometimes order locks by their memory address.
- **Lock-order checkers.** Tools such as ThreadSanitizer's deadlock
  detector or Linux's *lockdep* record the order in which each pair of
  locks is taken and warn if two orders conflict, even if no deadlock
  happened in that run.
- **Starvation is not deadlock.** In exercise m2 the explorer may report
  starvation: one philosopher keeps losing the race for a fork. Nobody is
  stuck forever as a group; the scheduler is just unfair to one of them.

## 13. Now do

| # | Exercise | Type | What you practise |
|---|---|---|---|
| 1 | `exercises/concept/ex1_rag_cycle.c` | warm-up | find a cycle in a resource-allocation graph |
| 2 | `exercises/concept/ex2_detect.c` | core | the detection algorithm with E, A, C, R |
| 3 | `exercises/concept/ex3_bankers.c` | core | `is_safe()` with a safe sequence, and the request check |
| 4 | `exercises/concept/ex4_bankers_from_memory.c` | from memory | the safety check without looking |
| 5 | `exercises/concept/ex5_lock_ordering.c` | core | three jobs, three locks: break circular wait |
| 6 | `exercises/concept/ex6_bank_transfer.c` | core | transfers between accounts, locks ordered by account id |
| m1 | `exercises/models/m1_slide14.il` | model | L06 slide 14 in the explorer |
| m2 | `exercises/models/m2_philosophers.il` | model | dining philosophers fixed by ordering the forks |

```bash
make test M=10                       # test all exercises
make test M=10 T=ex3_bankers         # test one exercise
make run  M=10 EX=two_locks_deadlock # break_it: watch a deadlock happen
make run  M=10 EX=banker ARGS="safe modules/10-deadlocks/calc/examples/safe.txt"
```

The calculator `calc/banker.c` has four commands: `safe`, `request`,
`detect` and `schedule`. Use it to check every table you work out by hand.

Then do the [quiz](quiz.md) and check yourself with the
[recall sheet](recall.md). Answers are in
[`answers/10-deadlocks/`](../../answers/10-deadlocks/), but try first.
