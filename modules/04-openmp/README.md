# 04 · OpenMP

| | |
|---|---|
| **Time** | about 5 hours |
| **You should know first** | module 03 (Pthreads: threads, blocks of work, mutexes, local accumulation) |
| **Lectures** | L04 *Shared Memory Programming with Pthreads & OpenMP* (slides 44–55, the OpenMP part). The slides are short, so most of section 8 is **[EXTRA]**. |
| **Helps with** | Lab 03 (all four exercises); exam questions like "parallelise this loop", "what does `reduction` do?", "can this loop run in parallel?", "which schedule would you use?" |

**Goal:** turn a serial C loop into a correct parallel loop with one or two
lines of OpenMP, choose the right way to protect shared results, decide
whether a loop *can* run in parallel at all, and use tasks for recursive
work like merge sort.

Words in **bold** link to the [glossary](../../GLOSSARY.md) the first time
they appear.

---

## 1. The problem

In module 03, summing an array with Pthreads needed: a struct type, a
thread function, the block arithmetic `t*n/p`, an array of `pthread_t`, a
loop of `pthread_create`, a loop of `pthread_join`, and a mutex. That is a
lot of code for "please run this loop on 4 cores", and every line is a
chance for a bug.

**OpenMP** does most of this for you. With OpenMP the same sum is:

```c
long s = 0;
#pragma omp parallel for reduction(+:s)
for (long i = 0; i < n; i++) s += a[i];
```

The compiler creates the threads, splits the loop into blocks, gives each
thread a private partial sum, and adds the partial sums at the end. (The
name means *Open Multi-Processing*; L04 slide 45.)

## 2. The idea

A `#pragma` is a special line that gives the compiler an extra instruction
(slide 45). OpenMP is a set of these lines, all starting with
`#pragma omp`, plus a few library functions such as
`omp_get_thread_num()`.

- You write ordinary serial C, then mark the parts that should run in
  parallel.
- You compile with `gcc -fopenmp`. **Without `-fopenmp`, gcc ignores every
  `#pragma omp` line** and the program quietly runs on one thread. With
  `-Wall` you at least get a warning ("ignoring #pragma omp parallel");
  see `make run M=04 EX=no_fopenmp`.
- OpenMP uses the same fork/join idea as module 03. At `#pragma omp
  parallel` the thread that was running (the *primary* thread, which the
  slides call the *master*) starts a **team** of threads. All of them run
  the block that follows. At the end of the block they all wait for each
  other, and then only the primary thread continues (slide 48).

```
            #pragma omp parallel
primary: ---+---- runs the block as thread 0 ----+---> continues alone
            +---- thread 1 runs the block -------+
            +---- thread 2 runs the block -------+   (everyone waits here
            +---- thread 3 runs the block -------+    for everyone else)
```

OpenMP only works on a **shared-memory** computer, where all threads see
the same variables. (For separate computers you need message passing:
module 13, MPI.)

## 3. The mechanism

### 3.1 A parallel region (L04 slides 46–50)

```c
#include <omp.h>
#include <stdio.h>

int main(void) {
#pragma omp parallel num_threads(4)
    {
        int id = omp_get_thread_num();          // 0, 1, 2 or 3
        int p = omp_get_num_threads();          // 4: the size of the team
        printf("hello from thread %d of %d\n", id, p);
    }
    return 0;
}
```

- `#pragma omp parallel` applies to the **one statement or `{ }` block**
  right after it. That block is called a *parallel region*. Every thread of
  the team runs all of it.
- `num_threads(4)` is a **clause**: extra words that change what a
  directive does (slide 49). Without it, OpenMP picks the number of threads
  (usually one per core, or the value of the `OMP_NUM_THREADS` environment
  variable).
- **The order of the lines is not fixed** (slide 47). Any order of the four
  `hello` lines can appear, just as in module 03.
- **You may get fewer threads than you asked for** (slide 50). The OpenMP
  standard does not promise `num_threads(4)` gives 4 threads, because a
  system can have limits. So code that splits work by hand must use the
  number it really got, `omp_get_num_threads()`, not the number it asked
  for. In practice you nearly always get what you ask for.

### 3.2 Shared or private?

This is the most important rule in OpenMP, and the source of most bugs.

| Where the variable is declared | Inside the region it is… | Example |
|---|---|---|
| **before** the parallel region (outside it) | **shared**: one variable, seen by every thread | `double total = 0;` above the pragma |
| **inside** the parallel region | **private**: each thread has its own | `int id = omp_get_thread_num();` |
| the loop counter of a `parallel for` loop | **private** automatically | `i` in `for (int i = ...)` |

A shared variable that several threads **write** is a race, exactly as in
module 03. You can also choose explicitly with clauses (section 8.2).

### 3.3 Mutual exclusion: `critical` (L04 slide 51)

```c
#pragma omp critical
global_result += my_result;
```

Only one thread at a time can run the statement (or `{ }` block) after
`#pragma omp critical`. It is a [**critical section**](../../GLOSSARY.md#critical-section-cs)
protected by a hidden [**mutex**](../../GLOSSARY.md#lock-mutex) that
OpenMP manages for you: no `lock`/`unlock` calls, so you cannot forget the
unlock.

### 3.4 The trapezoidal rule (L04 slides 52–55)

To estimate the area under a curve `f` between `a` and `b`, cut it into `n`
thin strips of width `h = (b - a) / n`. Treat each strip as a trapezoid:
strip `k` goes from `x_k = a + k*h` to `x_k+1`, and its area is
`(f(x_k) + f(x_k+1)) / 2 * h`. Add up all the strips.

The lecture parallelises it the "by hand" way:

1. Open a parallel region. Each thread finds its number and the team size.
2. Each thread works out its own block of strips (like the row blocks in
   module 03) and adds up their areas in a **private** variable.
3. Each thread adds its private sum to the shared `global_result` inside
   `#pragma omp critical`.

```c
double total = 0.0;                              // shared
#pragma omp parallel num_threads(nthreads)
{
    int id = omp_get_thread_num(), p = omp_get_num_threads();
    long first = (long)id * n / p, last = (long)(id + 1) * n / p;
    double mine = 0.0;                           // private
    for (long k = first; k < last; k++)
        mine += (f(a + k*h) + f(a + (k+1)*h)) / 2.0 * h;
#pragma omp critical
    total += mine;                               // one thread at a time
}
```

Why is the critical section needed? `total += mine` is load, add, store.
Two threads can load the same old `total`, and one thread's area is lost:

| step | thread 0 | thread 1 | `total` |
|--:|---|---|:--:|
| 1 | loads `total` (0.0) | | 0.0 |
| 2 | | loads `total` (0.0) | 0.0 |
| 3 | stores 0.0 + 1.1 | | 1.1 |
| 4 | | stores 0.0 + 7.9 | **7.9** (thread 0's 1.1 is lost) |

Note that there is only **one** critical section per thread, after the
loop: the same "accumulate locally, combine once" rule as module 03.
Exercise 1 does this, then does it again in one line with `reduction`
(section 8.3).

## 4. Why it works

- **The end of a parallel region is a barrier.** A *barrier* is a point
  where every thread waits until all threads of the team have arrived. So
  after the region, every thread's work is finished and visible, just like
  after `pthread_join` in module 03. That is why `return total;` after the
  region is safe.
- **`critical` gives [mutual exclusion](../../GLOSSARY.md#mutual-exclusion).**
  The load-add-store of `total += mine` runs in one thread at a time, so no
  update is lost.
- **Private variables cannot race.** Each thread has its own copy, so no
  other thread can read or write it.
- **`reduction` is local accumulation done for you** (section 8.3): private
  copies, then one combine per thread. That is why it is both correct and
  fast.

## 5. API cheat-sheet

| You write | What it does |
|---|---|
| `gcc -fopenmp ...`, `#include <omp.h>` | switch OpenMP on (without `-fopenmp` the pragmas are ignored) |
| `#pragma omp parallel` | start a team; every thread runs the next block |
| `num_threads(p)` | clause: ask for `p` threads |
| `omp_get_thread_num()` | my number in the team, `0 .. p-1` |
| `omp_get_num_threads()` | the size of the team (1 outside a parallel region) |
| `omp_set_num_threads(p)` / `OMP_NUM_THREADS=p` | the default team size for later regions |
| `omp_get_wtime()` | wall-clock time in seconds, for timing |
| `#pragma omp critical` | one thread at a time runs the next statement or block |
| `#pragma omp parallel for` / `#pragma omp for` | share the iterations of the next `for` loop [EXTRA] |
| `schedule(static / dynamic / guided / auto / runtime [, chunk])` | how iterations are given out [EXTRA] |
| `shared(x)`, `private(x)`, `firstprivate(x)`, `lastprivate(x)`, `default(none)` | data-sharing clauses [EXTRA] |
| `reduction(+:s)`, also `*`, `max`, `min`, `&&`, `\|\|` … | private copies, combined at the end [EXTRA] |
| `#pragma omp atomic` | one simple update like `x++` or `x += e`, done as one hardware step [EXTRA] |
| `#pragma omp barrier` | wait until every thread of the team gets here [EXTRA] |
| `#pragma omp single` / `#pragma omp master` | only one thread runs the block (`single`: any thread, everyone waits at the end; `master`: thread 0, nobody waits) [EXTRA] |
| `nowait` | clause: skip the barrier at the end of a `for` or `single` [EXTRA] |
| `#pragma omp task` / `#pragma omp taskwait` | package work for any thread of the team / wait for my child tasks [EXTRA] |

## 6. Common exam mistakes

- Forgetting `-fopenmp`, then wondering why there is no speedup. The
  pragmas were silently ignored.
- Declaring a work variable (a temporary, a running sum) **before** the
  region, which makes it shared, and then writing it from every thread.
- Writing `#pragma omp parallel` where you meant `#pragma omp parallel
  for`. With only `parallel`, **every** thread runs the **whole** loop, so
  the work is done `p` times.
- Putting `critical` around every addition inside the loop. It is correct
  but slower than serial. Use `reduction`, or a private sum plus one
  critical.
- Thinking OpenMP checks that a loop is safe to parallelise. It does not. If
  iteration `i` needs the result of iteration `i-1`, `parallel for` gives
  wrong answers without any warning (section 8.4).
- Assuming `num_threads(p)` always gives `p` threads (slide 50).
- Calling `rand()` from several threads. It shares one hidden state; use
  `rand_r(&my_seed)` with a private seed (Lab 03, Ex 2).

## 7. Careful: what the notes say

- **The OpenMP part of L04 is short (slides 44–55)** and its code is mostly
  pictures. It covers `parallel`, `num_threads`, `critical`, and the
  trapezoidal rule. Everything Lab 03 also needs (`parallel for`,
  schedules, `reduction`, `atomic`, `single`, tasks) is in section 8 and
  marked **[EXTRA]**.
- **L04 slide 48** calls the threads "master" and "slaves". Current OpenMP
  documents say *primary thread* and *worker threads*. They mean the same.
- **L04 slide 51 notes** mention `omp_set_lock(&writelock)`. That is
  OpenMP's explicit lock (`omp_lock_t`, with `omp_init_lock`,
  `omp_set_lock`, `omp_unset_lock`), the OpenMP version of a pthread mutex.
  You rarely need it: `critical`, `atomic` and `reduction` cover the usual
  cases.
- **Lab 03 compiles with `-O0`.** Our tests use `-O2`. Speedups at `-O0` are
  often larger, because each iteration does more (slow) work, so do not
  compare the two sets of numbers directly.

## 8. [EXTRA] Beyond the slides

### 8.1 `parallel for` and how iterations are divided

```c
#pragma omp parallel for num_threads(4)
for (int i = 0; i < 20; i++) arr[i] = i * i;
```

`#pragma omp parallel for` is short for a parallel region containing just
one `#pragma omp for`. `omp for` **shares** the iterations among the team:
each iteration runs exactly once, on some thread. The rules: the loop must
have the simple form `for (i = start; i < end; i += step)`, and the number
of iterations must be known when the loop starts (no `break`).

The `schedule` clause says how the iterations are given out. Here is the
real output of `make run M=04 EX=schedule_map` (20 iterations, 4 threads;
each digit is the thread that ran that iteration):

```
iteration     01234567890123456789
static        00000111112222233333     one block per thread, sizes as equal as possible
static, 2     00112233001122330011     blocks of 2, dealt out in turn like cards
dynamic       10231023102310231023     chunks of 1, taken by whichever thread is free
dynamic, 3    00033311122200033311     chunks of 3, taken by whichever thread is free
guided        00000222211133331213     big chunks first, getting smaller
```

| Schedule | How it works | Good for |
|---|---|---|
| `static` | iterations split into equal blocks *before* the loop starts, one per thread, in thread order. With 20 and 4: 5 each. With 10 and 4, GCC gives 3, 3, 2, 2. | every iteration costs the same (like the blur) — no extra cost at run time |
| `static, c` | blocks of `c` iterations, dealt out in turn: thread 0 gets the first block, thread 1 the second, and so on, then round again | cost changes smoothly along the loop |
| `dynamic, c` | each thread takes the next `c` iterations whenever it is free (default `c = 1`) | iterations of very different, unpredictable cost |
| `guided, c` | like dynamic, but the chunks start large (about remaining ÷ threads) and shrink to `c` | uneven cost, with less overhead than `dynamic, 1` |
| `auto` | the compiler and runtime choose | when you don't want to decide |
| `runtime` | read from `OMP_SCHEDULE` or `omp_set_schedule()` | trying several policies without recompiling |

`static` gives the same mapping on every run. `dynamic` and `guided` depend
on timing, so they change from run to run (Lab 03, Q1.2). Without a
`schedule` clause the choice is up to the compiler; GCC uses `static`.

**When does dynamic beat static?** (Lab 03, Q3.3.) When iterations have
very different costs. Example: testing whether each number up to 10 million
is prime. Big numbers take longer to test, so with `static` the thread with
the last block works much longer than the others, which sit idle. With
`dynamic, 64` a thread that finishes early simply takes more work. For the
blur, every row costs the same, so `static` is already balanced and
`dynamic, 1` only adds the cost of handing out 2000 tiny chunks.

### 8.2 Data-sharing clauses

| Clause | Meaning |
|---|---|
| `shared(x)` | one `x` for everyone (the default for variables declared outside) |
| `private(x)` | each thread gets a new `x` with **no starting value**. Reading it before writing it is a bug (gcc even warns "used uninitialized"). |
| `firstprivate(x)` | each thread gets its own `x`, **starting as a copy** of the outside `x` |
| `lastprivate(x)` | (on a `for`) private during the loop; afterwards the outside `x` gets the value from the **last iteration** (the one with the highest `i`) |
| `default(none)` | no automatic rules: you must list every variable as shared or private. The compiler then refuses any variable you forgot, which catches mistakes. |

After the region, the outside `x` is unchanged by `private` and
`firstprivate` (Lab 03, Q1.1). See it with
`make run M=04 EX=private_vs_firstprivate`.

### 8.3 `reduction`, `atomic` and `critical`

`reduction(+:count)` gives each thread a private `count` that starts at 0
(the starting value that does not change the result: 0 for `+`, 1 for `*`,
the smallest possible value for `max`). At the end, OpenMP combines the
private copies into the original `count`, once per thread. This is exactly
the "local accumulation" of Lab 01 Ex 3 Part B, done for you (Lab 03, Q2.2).

`#pragma omp atomic` protects **one** simple update (`x++`, `x += e`,
`x = x * e`, …) using a single hardware instruction instead of a lock. It is
cheaper than `critical`, but if all threads update the same variable all
the time, they still queue for it.

Lab 03 Ex 2 measures all four approaches. Our run (10,000,000 darts, 4
threads, `-O2`; yours will differ):

| Approach | pi | time (s) | Why |
|---|--:|--:|---|
| A `critical` around `count++` | 3.141520 | 0.63 | a lock taken for every hit: about 7.9 million times |
| B `atomic` on `count++` | 3.141520 | 0.28 | no lock, but all threads update one variable millions of times |
| C `reduction(+:count)` | 3.141520 | **0.02** | private counts, combined 4 times |
| D one counter per thread, added after | 3.141520 | 0.05 | same idea as C, written by hand |

All four give the same estimate: they use the same seeds and the same split
of the loop. The ranking is what matters: protecting every update costs far
more than the work itself.

### 8.4 Loop-carried dependences: can this loop run in parallel?

A loop can run in parallel only if its iterations are **independent**: no
iteration needs a value that another iteration writes. When iteration `i`
needs a result of an *earlier* iteration, the loop has a **loop-carried
dependence**.

**How to recognise one.** Look for a variable that is written in one
iteration and read in a later one:

```c
for (i = 1; i < n; i++) a[i] = a[i-1] + i;      // reads a[i-1], written by iteration i-1
for (i = 0; i < n; i++) b[i] = 2 * a[i];         // fine: each b[i] uses only a[i]
for (i = 0; i < n; i++) { t = a[i] * 2; b[i] = t + 1; }   // fine IF t is private
for (i = 0; i < n; i++) sum += a[i];             // a dependence on sum, but reduction removes it
```

The third loop only *looks* dependent: `t` is just scratch space. Make it
private (declare it inside the loop) and the iterations are independent.
The fourth is a dependence through `sum`, but addition can be done in any
order, so `reduction` handles it.

**What goes wrong if you ignore it.** `make run M=04 EX=loop_dependence`
puts `parallel for` on the first loop. With 4 threads, about 75% of the
entries come out wrong: each thread starts its block from a value of
`a[i-1]` that the previous thread has not computed yet.

**Ways to remove a dependence:**
1. **Find a formula (a "closed form").** `a[0] = 0; a[i] = a[i-1] + i` is
   `0, 1, 3, 6, 10, …`, which is `a[i] = i(i+1)/2`. Each `a[i]` now needs
   only `i`, so `#pragma omp parallel for` is safe. Exercise 3.
2. **Use a parallel prefix sum (scan).** For a general
   `out[i] = out[i-1] + in[i]`, there is no formula, but a two-pass method
   works: each thread adds up its own block, the block totals are turned
   into starting offsets, and each thread adds its offset to its block.
   Exercise 2. It does about twice as many additions as the serial loop,
   but they run in parallel.
3. **Use a reduction** when only the final total is needed.

**When a loop is not parallelisable.** If each step needs the previous
result and there is no formula or reordering trick, the loop must run in
order. Example: `p = f(p, i)` for a general function `f`, such as one step
of a simulation that depends on the previous state, or following a linked
list (`p = p->next`). You cannot know the 1000th value without computing
the 999 before it.

### 8.5 `barrier`, `single`, `master`, `nowait`

Inside one parallel region you often need some steps done by one thread, or
all threads in step:

- `#pragma omp barrier`: every thread waits here until the whole team has
  arrived. The prefix sum needs one between "compute block totals" and
  "use the offsets".
- `#pragma omp single`: the next block runs on **one** thread (whichever
  gets there first). The others wait at the end of the block (an implied
  barrier).
- `#pragma omp master`: the next block runs on thread 0 only, and **nobody
  waits**. (Newer OpenMP versions call it `masked`.)
- `nowait`: removes the implied barrier at the end of a `for` or `single`,
  when you know the threads do not need to wait.

`omp for`, `single` and the whole parallel region end with an implied
barrier; `master` and `critical` do not.

### 8.6 Tasks (Lab 03, Ex 4)

`parallel for` needs a loop whose size is known in advance. Divide-and-
conquer algorithms (merge sort, quicksort, tree walks) create work as they
go. **Tasks** handle this:

```c
#pragma omp parallel          // create the team
#pragma omp single            // ONE thread starts the recursion
sort(a, 0, n - 1, 0);

void sort(int *a, int lo, int hi, int depth) {
    if (hi - lo is small || depth >= LIMIT) { serial_sort(a, lo, hi); return; }   // cutoff
    int mid = lo + (hi - lo) / 2;
#pragma omp task
    sort(a, lo, mid, depth + 1);          // "this piece of work may be run by any thread"
#pragma omp task
    sort(a, mid + 1, hi, depth + 1);
#pragma omp taskwait                      // wait for MY two child tasks
    merge(a, lo, mid, hi);
}
```

- `#pragma omp task` packages the next statement as a piece of work. Any
  idle thread of the team may run it, now or later.
- `#pragma omp taskwait` waits until the tasks created by this call are
  finished. Without it, `merge` could run on halves that are not sorted yet.
- **Why `single`?** (Lab 03, Q4.3.) Every thread of a team runs the
  parallel region. Without `single`, all threads would start their own
  complete sort of the same array, at the same time, on the same memory:
  wasted work and wrong results. With `single`, one thread starts it, and
  the others wait at the end of `single`. While waiting, they run the
  tasks it creates.
- **Why a cutoff?** (Lab 03, Q4.2.) Each task has a fixed cost: creating
  it, queueing it, and handing it to a thread. Sorting 4 numbers costs far
  less than that. On our machine, sorting 100,000 numbers with tasks all
  the way down (depth limit 30) took 0.07–0.13 s, against 0.009 s for the
  serial sort: up to 14 times **slower**. With depth limit 10 and 10 million
  numbers, 8 threads gave a speedup of about 3.6. The size of the work in
  one task is called its **granularity**: it must be big enough to be worth
  the overhead.
- Variables in a task are `firstprivate` by default (each task gets a copy
  of the values when it is created). If a task must hand a result back, as
  in exercise 4, mark that variable `shared(...)` and read it only after
  `taskwait`.

### 8.7 False sharing (Lab 03 Ex 2, approach D)

Approach D gives each thread a counter `local[id]` in one array. The
counters are different variables, so there is no race. But the hardware
moves memory between cores in blocks of 64 bytes (*cache lines*). If two
threads' counters sit in the same 64-byte block, every write by one thread
forces the block away from the other thread's core. The program stays
correct but gets much slower. This is called **false sharing**. The fix is
padding: put each counter in its own 64-byte block (the lab-track answer
does this), or count in a local variable and write the array once at the
end. Module 06 (`ex6_false_sharing`) measures the effect.

### 8.8 ThreadSanitizer and OpenMP

`make tsan` works well for Pthreads (module 03) but **not for OpenMP**. The
OpenMP runtime library (libgomp) is not built for ThreadSanitizer, so TSan
cannot see the synchronization inside it (the barriers, the reduction
combine) and reports races that are not real. So the module 04 tests skip
themselves under `make tsan`. To check OpenMP code for races, rely on
careful shared/private reasoning, `default(none)`, and repeated runs with
many threads (which the tests do).

## 9. Now do

| # | Exercise | Type | What you practise |
|---|---|---|---|
| 1 | `exercises/concept/ex1_trapezoid.c` | warm-up | the lecture's trapezoidal rule with `critical`, then with `reduction` |
| 2 | `exercises/concept/ex2_prefix_sum.c` | core [EXTRA] | parallel prefix sum: blocks, `barrier`, `single` |
| 3 | `exercises/concept/ex3_closed_form.c` | core [EXTRA] | remove a loop-carried dependence with a formula |
| 4 | `exercises/concept/ex4_task_sum.c` | core [EXTRA] | divide and conquer with `task`, `taskwait`, `single`, a cutoff |
| 5 | `exercises/concept/ex5_reduction_from_memory.c` | from memory | `parallel for` + `reduction(+)` and `reduction(max)` |
| L1 | `exercises/lab/lab1_warmup.c` | lab track | Lab 03 Ex 1: hello, `firstprivate`, which thread runs which iteration |
| L2 | `exercises/lab/lab2_montecarlo_pi.c` | lab track | Lab 03 Ex 2: Monte Carlo pi with critical / atomic / reduction / per-thread counters |
| L3 | `exercises/lab/lab3_blur.c` | lab track | Lab 03 Ex 3: box blur with four schedules, speedup tables |
| L4 | `exercises/lab/lab4_mergesort.c` | lab track | Lab 03 Ex 4: merge sort with tasks and a depth cutoff |

```bash
make test M=04                              # test all exercises
make test M=04 T=lab2_montecarlo_pi         # test one exercise
make run  M=04 EX=lab2_montecarlo_pi        # the Lab 03 Ex 2 tables
make run  M=04 EX=lab3_blur                 # the Lab 03 Ex 3 tables
make run  M=04 EX=lab4_mergesort ARGS="100000 30"   # Lab 03 Q4.2: no cutoff
make run  M=04 EX=schedule_map ARGS="20 4"  # which thread runs which iteration
make run  M=04 EX=no_fopenmp                # what happens without -fopenmp
make run  M=04 EX=private_vs_firstprivate   # Lab 03 Q1.1
make run  M=04 EX=loop_dependence           # parallel for on a dependent loop
```

Then do the [quiz](quiz.md) and check yourself with the
[recall sheet](recall.md). Answers are in
[`answers/04-openmp/`](../../answers/04-openmp/), but try first.
