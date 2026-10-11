# 11 · Parallel patterns & design ★

| | |
|---|---|
| **Time** | about 5 hours |
| **You should know first** | module 02 (speedup, work and span), module 03 (pthreads), module 04 (OpenMP), module 08 (condition variables and the bounded buffer) |
| **Lectures** | L01 *Introduction* (slides 17–22: splitting the prime search); L02 *How Much Parallelism?* (slides 14–19 and 26–27: P-FIB, spawn/sync, throughput); L10 *MPI* (slide 14: SPMD; slide 43: block, cyclic and block-cyclic partitioning). Most of the patterns themselves come from **Lab 06**, not from a lecture. |
| **Helps with** | Lab 06, all exercises; exam questions like "design a pipeline and work out its time", "how many threads for this job?", "the input does not fit in memory", "threads fight over one output file" |

**Goal:** given a problem, pick a sensible way to split it between threads,
explain *why* in two or three sentences, and code the common patterns:
parallel loops, divide and conquer, pipelines, thread pools and work
stealing.

Words in **bold** link to the [glossary](../../GLOSSARY.md) the first time
they appear.

---

## 1. The problem

You have a 4-core machine and a slow program. "Use threads" is not yet a
plan. You still have to decide three things:

1. **Who does which piece of work?**
2. **What happens when some pieces take much longer than others?**
3. **Where do the results go, without the threads waiting for each other?**

Here is what goes wrong if you decide badly. L01 (slides 17–19) wants to
print every prime below 10¹⁰ with 10 threads. The first idea: thread 0 tests
the first tenth of the numbers, thread 1 the second tenth, and so on. Every
thread gets the same *number* of items. But big numbers take longer to test,
so the threads get very different amounts of *work*. The thread with the
biggest numbers finishes last, and the others sit idle.

We measured the same idea on our machine with `ex2_parallel_for` (count the
primes below 2 million, 4 threads). "Imbalance" is the busiest thread's work
divided by the average work; 1.00 is perfect.

| how the numbers were split | imbalance | speedup on 4 threads |
|---|--:|--:|
| one block of consecutive numbers per thread | 1.33 | 2.5 |
| dealt out like cards (thread *i* gets every 4th number) | 1.99 | 1.7 |
| threads grab the next 1000 numbers when they are free | 1.02 | 3.5 |

(Your times will differ. The imbalance column does not depend on the machine.)

The middle row is a surprise, and section 3.3 explains it. The lesson: **the
way you split the work matters as much as the number of threads.**

## 2. The idea: a small catalogue of patterns

A **pattern** is a standard way to organise a parallel program. Each one fits
a certain *shape* of problem. Learn the shape, and you can recognise it in a
new problem.

| Pattern | Shape of the problem | Picture | Example | In the notes? |
|---|---|---|---|---|
| **Loop parallel** | the same work for every item; items do not depend on each other | `for i in 0..n-1: f(i)`, iterations shared out | every pixel of an image | L04 (OpenMP `parallel for`), Lab 03, Lab 06 Ex 1 |
| **SPMD** (single program, multiple data) | every thread runs the same code on its own part of the data, choosing its part by its number | `my part = part(rank)` | MPI programs, ex3 | L10 slide 14 |
| **Divide & conquer / fork-join** | the problem splits into smaller copies of itself | a tree of sub-problems | sum of a tree, merge sort, P-FIB | L02 slides 14–19 |
| **Pipeline** | every item goes through the same fixed *steps*, in order | `stage 1 → stage 2 → stage 3` | read → decode → save, for each frame of a video | Lab 06 Ex 3 **[EXTRA]** to the slides |
| **Master–worker / thread pool** | many independent *tasks* of different sizes | workers take tasks from one shared queue | a web server handling requests | L01 slides 20–22 (shared counter), Lab 06 Ex 4 |
| **Work stealing** | like a thread pool, but many small tasks (or tasks that create tasks) | one queue per worker; idle workers steal | a parallel web crawler | Lab 06 Ex 5 **[EXTRA]** |
| **Map-reduce** | do something to every item, then combine the answers | map in parallel, then reduce | count lines with "ERROR" in a log | **[EXTRA]** |

**How to choose.** Ask these questions in order:

1. *Is it the same small computation for every item, with no item needing
   another item's result?* → loop parallel (or SPMD). Then ask: do the items
   cost about the same? If yes, **static** split; if no, **dynamic**.
2. *Does the problem break into smaller copies of itself?* → divide &
   conquer, with a **threshold** below which you stop splitting.
3. *Does every item pass through several different steps in a fixed order?*
   → pipeline. Find the slowest step; give it more threads.
4. *Is it a stream of separate tasks of very different sizes?* → thread pool
   with a shared queue. Very many small tasks, or tasks that make new tasks?
   → work stealing.
5. *Do you need one combined answer (a total, a count, a maximum)?* → let
   every thread compute a private partial answer, and combine at the end
   (reduce). Never update one shared total for every item.

## 3. The mechanism

### 3.1 Loop parallel: when can a loop run in parallel?

A loop can be split between threads **if its iterations can run in any order
and still give the same result**. Look at what each iteration reads and
writes.

```c
for (i = 0; i < n; i++) b[i] = 2 * a[i];         // (1) OK: iteration i touches only a[i], b[i]
for (i = 0; i < n; i++) sum += a[i];             // (2) OK as a REDUCTION (see below)
for (i = 1; i < n; i++) a[i] = a[i-1] + x[i];    // (3) NOT OK: needs the previous iteration's result
for (i = 0; i < n-1; i++) a[i] = a[i+1];         // (4) NOT OK as written: see below
```

- **(1)** Each iteration uses its own elements only. Any order works.
- **(2)** Every iteration updates the same `sum`. Done in parallel, that is a
  [**race condition**](../../GLOSSARY.md#race-condition-data-race). But
  addition can be regrouped, so each thread can add up its own part in a
  private variable and the parts are added at the end. OpenMP does this for
  you with `reduction(+:sum)` (module 04).
- **(3)** Iteration *i* needs the value that iteration *i − 1* just wrote. This
  is a **loop-carried dependence**: a value flows from one iteration to a
  later one. You cannot simply split this loop. (A running total like this
  *can* be parallelised with a cleverer two-pass algorithm, called a parallel
  prefix sum **[EXTRA]**, but not by adding a pragma.)
- **(4)** Iteration *i* reads `a[i+1]`, which iteration *i + 1* overwrites. If
  iteration *i + 1* runs first, iteration *i* reads the new value instead of
  the old one. Fix: read from a copy (`b[i] = a[i+1]` into a new array).

**Quick test:** if one iteration *writes* a variable that a *different*
iteration *reads or writes*, the loop is not safe to split as it is.

### 3.2 SPMD and data decomposition: who gets which items?

In **SPMD** (L10 slide 14) every thread or process runs the same program. It
asks "what is my number (rank)?" and uses that to pick its part of the data.
The rule for picking is a **partition**. L10 slide 43 names three. Here are
10 items shared by 4 threads (checked with `make run M=11 EX=ex1_partition
ARGS="10 4 2" ANS=1`):

```
item:           0  1  2  3  4  5  6  7  8  9
block:          0  0  0  1  1  1  2  2  3  3     each thread: one run of neighbours
cyclic:         0  1  2  3  0  1  2  3  0  1     dealt out like cards
block-cyclic:   0  0  1  1  2  2  3  3  0  0     blocks of b = 2, dealt out like cards
```

| Partition | Owner of item *i* | Good when | Bad when |
|---|---|---|---|
| **block** | thread *r* gets *n/p* items, plus one extra if *r* < *n mod p* | neighbouring items are used together (good for the cache); all items cost the same | cost grows along the array (the last block is the heaviest) |
| **cyclic** | *i mod p* | cost changes smoothly along the array | neighbouring items are used together; the data has a pattern with the same period as *p* (section 3.3) |
| **block-cyclic** | *(i / b) mod p* | a compromise: blocks keep neighbours together, dealing spreads the cost | |

The rule "the first *n mod p* threads get one extra item" keeps every block
within one item of the others. With *n* = 10 and *p* = 4: 10 / 4 = 2 and
10 mod 4 = 2, so threads 0 and 1 get 3 items and threads 2 and 3 get 2.

### 3.3 Load balancing: static vs dynamic

**Load balancing** means giving every thread about the same amount of
*work*, so that they all finish at about the same time. The program is only
as fast as its slowest thread.

- **Static** load balancing decides the split **before** the work starts
  (block or cyclic, `schedule(static)` in OpenMP). It costs nothing at run
  time, but it only works if you can predict the cost of each item.
- **Dynamic** load balancing hands out work **while** the program runs. A
  free thread takes the next piece from a shared counter or queue
  (`schedule(dynamic)` in OpenMP; L01 slides 20–22 use a shared counter
  `getAndIncrement()`). It costs one atomic operation per piece, but no
  thread sits idle while work is left.

**Irregular work** is where dynamic wins. Two examples:

- *Prime testing* (L01): big numbers take longer.
- *Mandelbrot* (Lab 06 Ex 1): pixels inside the black shape run the full
  `max_iter` steps; pixels far outside escape after a few. In our run of
  `lab1_mandelbrot` with 4 static threads, the work per thread was about
  3.6, 82.8, 83.4 and 3.7 million steps. The two middle threads do almost
  everything, so 4 threads are hardly faster than 2. With `schedule(dynamic)`
  the speedup kept growing with more threads.

**Why cyclic was so bad for primes.** With 4 threads, cyclic gives thread 0
the numbers 0, 4, 8, 12, … and thread 2 the numbers 2, 6, 10, … All of these
are even, so each test stops after one division. Threads 1 and 3 get all the
odd numbers, which hold all the real work. The pattern in the data (even and
odd) has the same period as the deal. **Any static rule can be unlucky like
this.** Dynamic scheduling cannot, because it never looks at the numbers in
advance.

**Chunk size.** Taking one item at a time balances best, but every take is
an atomic operation on a shared counter, and all threads hit that same
counter. Taking a chunk (say 1000 items) at a time cuts that cost by 1000.
Chunks that are too big bring back the imbalance at the end. In practice:
many more chunks than threads, but each chunk much bigger than the cost of
taking it.

### 3.4 Divide & conquer and fork-join, with a threshold

**Divide & conquer:** split the problem into smaller problems of the same
kind, solve them (in parallel), combine the results. **Fork-join** is how it
runs: a thread *forks* (starts) a child for one half, works on the other half
itself, then *joins* (waits for) the child. L02 slide 16 writes this as
`spawn` and `sync` in P-FIB:

```
tree_sum(n):                           in pthreads:
    if n is small: return serial sum   (below the threshold)
    x = spawn tree_sum(n.left)         pthread_create(child, left subtree)
    y = tree_sum(n.right)              meanwhile, do the right side here
    sync                               pthread_join(child)
    return n.value + x + y
```

Starting a thread costs tens of microseconds. For a subtree of 10 nodes,
that is far more than adding the 10 numbers. So we only fork when the
subtree is bigger than a **threshold** (also called a *cutoff*). Our run of
`lab2_tree_sum` (2.7 million nodes, 16 cores; your numbers will differ):

| threshold | threads started | speedup |
|--:|--:|--:|
| 100 | 39 864 | 0.38 (slower than serial!) |
| 1 000 | 4 022 | 2.8 |
| 10 000 | 405 | 7.0 |
| 100 000 | 40 | 7.8 |
| 1 000 000 | 3 | 3.1 |

- **Threshold too low:** tens of thousands of threads. Creating and joining
  them costs more than the work they do.
- **Threshold too high:** only a few threads, so most cores sit idle. The
  tree is unbalanced, so the few big pieces are also uneven.
- **Just right:** a few times more pieces than cores, each one big enough to
  hide the cost of its thread.

This matches L02's idea of *parallelism* = work / span (slide 26): the
threshold controls how much parallelism you actually create.

### 3.5 Pipeline **[EXTRA]** to the slides (Lab 06 Ex 3)

A **pipeline** splits the processing of each item into **stages**. Each stage
is its own thread (or machine), and stages pass items along through
**bounded queues** (module 08's bounded buffer). Like an assembly line, one
item does not get faster, but several items are worked on at the same time.

```
            q12 (max 16)              q23 (max 16)
 stage 1 ─────────────> stage 2 ─────────────> stage 3
 2 ms per item          6 ms per item          2 ms per item
```

Why **bounded** queues? If stage 1 is faster than stage 2, items pile up in
`q12`. An unbounded queue would grow until memory runs out. With a bound,
stage 1 simply waits when `q12` is full.

**Timing.** Say each item needs 2 ms, then 6 ms, then 2 ms, and there are
100 items.

- **One thread doing everything:** 100 × (2 + 6 + 2) = **1000 ms**.
- **Pipeline:** the first item comes out after 2 + 6 + 2 = 10 ms. After
  that, stage 2 can finish only one item every 6 ms, so one item comes out
  every 6 ms. Total ≈ 10 + (100 − 1) × 6 = **604 ms**.
- The general rule (same model as the exam):
  **total ≈ (sum of stage times) + (n − 1) × (slowest stage time)**.
  With *k* equal stages of time *t* this becomes **(k + n − 1) × t**:
  *k* steps to fill the pipeline, then one item per step.
- **Throughput** (items per second) is set by the slowest stage alone:
  1 item / 6 ms ≈ 167 items per second. The slowest stage is the
  **bottleneck**.
- **Replicate the bottleneck:** run 3 copies of stage 2. Together they finish
  one item every 6 / 3 = 2 ms, the same pace as the other stages. Total ≈
  10 + 99 × 2 = **208 ms**. A fourth copy would not help: stages 1 and 3
  are now the bottleneck at 2 ms.

`make run M=11 EX=pipeline_calc ARGS="100 2 6/3 2"` prints this working.

**Two traps when you replicate a stage** (both trained in `lab3_pipeline`):

1. **Order.** Two stage-2 threads can overtake each other, so items reach
   stage 3 out of order. If order matters, give each item a sequence number
   and let stage 3 hold early items back until their turn (a *reorder
   buffer*).
2. **Stopping.** Stage 1 says "no more items" with a special **poison pill**
   item. Each stage-2 thread stops after taking *one* pill, so with *k*
   stage-2 threads stage 1 must send *k* pills. Each stage-2 thread passes
   its pill on, and stage 3 stops after counting *k* pills. Send too few and
   some thread waits forever.

### 3.6 Master–worker and the thread pool (Lab 06 Ex 4)

A **thread pool** starts a fixed number of worker threads once. Work goes
into a shared queue. Each worker loops: take a task, run it, repeat. Fast
workers simply take more tasks, so uneven tasks balance out by themselves.
And we do not pay to create a thread for every task.

The simplest version is L01's shared counter: tasks sit in an array, and a
counter says which one is next (Lab 06 Ex 4, `run_counter_pool`):

```c
for (;;) {
    lock(m);  i = next++;  unlock(m);   // take a task: a short critical section
    if (i >= n) break;
    do_task(&tasks[i]);                 // the long work runs WITHOUT the lock
}
```

A general pool (`pool_create / pool_submit / pool_wait / pool_destroy` in
`lab4_thread_pool`) uses a linked-list queue, one mutex and two condition
variables (module 08):

- workers wait on `work_ready` while the queue is empty;
- a counter `pending` = tasks submitted but not finished. `pool_wait` waits on
  `all_done` until it reaches 0. Increase it in `submit`, **not** when a
  worker starts the task. Otherwise a running task that is about to submit
  more tasks could let `pending` touch 0 too early;
- to shut down, set a `shutdown` flag and **broadcast** `work_ready`. A
  single signal would wake only one sleeping worker, and the others would
  sleep forever.

The **master–worker** pattern is the same idea with a special *master*
thread that creates the tasks and collects the results.

### 3.7 Work stealing **[EXTRA]** to the slides (Lab 06 Ex 5)

With one shared queue, every worker locks the **same** mutex for every task.
With many workers and tiny tasks, they queue up for that lock. Work stealing
gives every worker its **own** double-ended queue (a *deque*) with its own
lock:

```
worker 0's deque:   front [ t0  t4  t8  t12 ] back   <- worker 0 pushes and pops HERE
                      ^
                      thieves steal HERE
```

- A worker pushes and pops at the **back** of its own deque. Usually nobody
  else touches it, so its lock is almost never contended.
- Only when its deque is empty does it become a *thief*. It picks another
  worker at random and steals **one** task from the **front** of that deque.

**Why opposite ends?**

1. *Less fighting.* The owner and a thief want the same item only when one
   item is left. Most of the time they work at different ends.
2. *Cache.* The newest task (at the back) was usually created most recently
   by the owner, so its data is probably still in the owner's cache. The
   owner keeps those.
3. *Bigger steals.* In divide & conquer, the oldest tasks (at the front) are
   the biggest ones, made near the top of the tree. One steal then gives the
   thief a lot of work, so it does not have to steal again soon.

**When can a worker stop?** In `lab5_work_stealing` tasks never create new
tasks. So once a deque is empty it stays empty, and a worker may stop after
one steal attempt on every other deque fails. If tasks *could* create tasks,
an empty deque might refill later. You would then need a shared count of
unfinished tasks to know when everything is done.

### 3.8 Map-reduce **[EXTRA]**

**Map-reduce** has two phases. *Map:* apply a function to every item
independently (a parallel loop). *Reduce:* combine the results into one
answer with an operation like +, max or "append".

Example: count the lines that contain "ERROR" in a huge log file. Map: for
each line, 1 if it contains ERROR, else 0. Reduce: add them up. In a shared-
memory program, each thread reduces its own part into a private count, and
the private counts are added at the end. That is exactly what OpenMP's
`reduction` clause and `ex3_chunked_scan` do. Big frameworks (Hadoop, Spark)
do the same across many machines.

## 4. Why the codes are correct

- **Dynamic scheduling hands out every item exactly once.** The take is one
  `atomic_fetch_add(&next, chunk)`. It returns the old value and adds in one
  [**atomic**](../../GLOSSARY.md#atomic) step, so two threads can never get
  the same start. A separate read and write would let two threads read the
  same value (the `counter++` race of module 01; L01 slides 22–24 show it).
- **The pool's lock is held only for the take, never for the task.** Holding
  it while running a task would make the workers take turns, and the pool
  would be no faster than one thread.
- **`pool_wait` cannot return early.** `pending` counts a task from the
  moment it is submitted until it has finished. A task that submits children
  is still counted while it submits them, so `pending` stays above 0 until
  the whole family is done.
- **The pipeline cannot lose or double an item.** Each item is put into a
  queue once and taken once, and the queue is protected by its mutex. Each
  stage-2 thread takes exactly one pill. So *k* pills stop exactly *k*
  threads, and stage 3 sees exactly *k* pills, all after the last real item
  that the corresponding thread passed on.
- **Work stealing hands out every task exactly once.** Pop and steal both
  change `front` or `back` while holding the deque's lock. So for one
  deque, all takes happen one at a time, and each index is taken once.

## 5. Design reasoning: the exam's favourite questions

Exam papers often describe a system and ask you to justify design choices
(see [exam/README.md](../../exam/README.md)). Here are our own scenarios with
model reasoning. Always give the *reason*, not just the number.

**How many threads on a 4-core machine?**
- *CPU-bound* job (all computing, for example resizing photos already in
  memory): about **4**, one per core. More threads cannot run at the same
  time; they only add switching and cache misses.
- *I/O-bound* job (each request waits for a disk or the network most of the
  time): **more than 4**. While one thread waits, another can use the core.
  A rule of thumb **[EXTRA]**: threads ≈ cores × (1 + wait time / compute
  time). If each task computes for 10 ms and waits for 90 ms:
  4 × (1 + 9) = **40** threads.

**The input file is 100 GB, the machine has 8 GB of memory.** Do not load it
all. Read it in **chunks** (say 256 MB). Process one chunk with all threads,
then reuse the buffer for the next one, so memory use stays bounded. Watch
the chunk edges: a chunk usually ends in the middle of a line. Process only
up to the last complete line and carry the rest into the next chunk
(`ex3_chunked_scan`, part B). Reading the next chunk while the threads work
on the current one turns this into a small pipeline (reader → workers).

**All threads append their results to one shared output file.** Every write
needs the same lock (or the lines get mixed up). So the threads spend their
time queueing for that lock, and writing becomes the bottleneck (L12 slide
5: the code under a lock runs one thread at a time). Better: each thread
writes into its **own buffer** in memory (or its own temporary file), and the
buffers are merged at the end, or written in large blocks. This also keeps
the output in a predictable order (`ex3_chunked_scan`, part A).

**A big table that every task only reads** (a dictionary, a model, a price
list). **Load it once**, before the threads start, and share it. Data that
nobody writes needs no lock, because a race needs at least one writer.
Loading it inside every task would waste time and memory. Loading it lazily
("the first thread that needs it loads it") would need extra
synchronization.

## 6. API cheat-sheet

| You want | Write | Meaning |
|---|---|---|
| a parallel loop, equal-cost items | `#pragma omp parallel for schedule(static)` | blocks of iterations fixed before the loop starts |
| a parallel loop, uneven items | `#pragma omp parallel for schedule(dynamic, 16)` | free threads grab the next 16 iterations |
| set the thread count for one loop | `... num_threads(p)` | |
| a sum across iterations | `reduction(+:sum)` | private copies, added at the end |
| fork / join | `pthread_create(&t, NULL, fn, arg)` / `pthread_join(t, NULL)` | after the join you see everything the child wrote |
| take the next piece of work | `start = atomic_fetch_add(&next, chunk)` | returns the old value; one atomic step |
| wait until a condition holds | `while (!cond) pthread_cond_wait(&cv, &m);` | always in a `while` loop, with `m` locked |
| wake every waiting thread | `pthread_cond_broadcast(&cv)` | use it for shutdown, and whenever several may need to wake |

## 7. Common exam mistakes

- **Equal items are not equal work.** Static block splitting is only
  balanced when every item costs the same.
- **Cyclic is not automatically fair.** It fails when the data has a pattern
  with the same period as the number of threads (even and odd numbers).
- **A pipeline is not faster per item.** It improves *throughput*, not the
  time for one item. Its speed is set by the slowest stage.
- **Pipeline time:** do not forget the *fill* time. The total is
  (sum of stages) + (n − 1) × (slowest), not n × (slowest).
- **More threads are not always better.** For CPU-bound work, more threads
  than cores do not help; for tiny tasks, thread creation can cost more than
  the task.
- **Running the task while holding the queue's lock** makes a pool sequential.
- **Poison pills:** one per consumer thread, not one in total.
- **"The loop has no shared variable, so it is safe"** is not enough. Check
  whether one iteration writes something that another iteration reads
  (`a[i] = a[i+1]`).

## 8. Careful: what the notes say

- **There is no lecture on patterns.** Lab 06 Ex 6 asks you to choose "the
  most suitable parallel pattern from the lecture", but the exported slides
  never list these patterns together. Only fragments exist: load balancing
  in L01, spawn/sync in L02, SPMD and partitioning in L10. Sections 3.5–3.8
  are therefore marked **[EXTRA]** to the slides, but they are exactly what
  Lab 06 practises. (Proposed for `NOTES_ERRATA.md`.)
- **L10 slide 43:** the pictures for block, cyclic and block-cyclic
  partitioning are images that did not survive the export. Section 3.2
  redraws them.
- **L01 slides 17–21:** "1010" and "109" are 10¹⁰ and 10⁹ (the exponents
  were lost in the export; this is already in `NOTES_ERRATA.md`).
- **OpenMP `schedule(static)`** without a chunk size splits the iterations
  into "about equal" blocks. The exact split is left to the compiler. GCC
  gives the first *n mod p* threads one extra iteration, which is the rule
  used in this module.

## 9. Now do

| # | Exercise | Type | What you practise |
|---|---|---|---|
| 1 | `exercises/concept/ex1_partition.c` | warm-up | block, cyclic and block-cyclic index formulas |
| 2 | `exercises/concept/ex2_parallel_for.c` | core | static block, static cyclic and dynamic scheduling with pthreads; the prime example |
| 3 | `exercises/concept/ex3_chunked_scan.c` | challenge | a file bigger than memory, per-thread buffers, merging in order |
| 4 | `exercises/concept/ex4_dynamic_from_memory.c` | from memory | dynamic scheduling with a shared counter |
| L1 | `exercises/lab/lab1_mandelbrot.c` | Lab 06 Ex 1 | OpenMP static vs dynamic; measuring the imbalance |
| L2 | `exercises/lab/lab2_tree_sum.c` | Lab 06 Ex 2 | fork-join with a threshold; the threshold table |
| L3 | `exercises/lab/lab3_pipeline.c` | Lab 06 Ex 3 | 3 stages, bounded queues, replicated stage 2, poison pills, reordering |
| L4 | `exercises/lab/lab4_thread_pool.c` | Lab 06 Ex 4 | the shared-index pool, then a real pool with submit/wait/destroy |
| L5 | `exercises/lab/lab5_work_stealing.c` | Lab 06 Ex 5 | per-thread deques, pop from the back, steal from the front |
| calc | `calc/pipeline_calc.c` | tool | prints a worked pipeline-time solution |

Lab 06 Ex 6 (choosing a pattern) is practised in [quiz](quiz.md) section C.

```bash
make test M=11                              # test all exercises
make test M=11 T=lab3_pipeline              # test one exercise
make run  M=11 EX=lab1_mandelbrot           # run it: prints the Lab 06 tables
make run  M=11 EX=pipeline_calc ARGS="100 2 6/3 2"
make tsan M=11                              # look for data races
```

Then do the [quiz](quiz.md) and check yourself with the
[recall sheet](recall.md). Answers are in
[`answers/11-parallel-patterns/`](../../answers/11-parallel-patterns/), but
try first.
