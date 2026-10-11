# 03 · Pthreads

| | |
|---|---|
| **Time** | about 5 hours |
| **You should know first** | module 00 (what a thread is), module 01 (what a race condition is); C pointers, structs and `malloc` |
| **Lectures** | L04 *Shared Memory Programming with Pthreads & OpenMP* (slides 1–43, the Pthreads part) |
| **Helps with** | Lab 01 Ex 1 and Ex 3; exam questions like "write the Pthreads code that…", "why is busy-waiting slow here?", "which lock would you use for this list?"; module 04 (OpenMP) |

**Goal:** write a Pthreads program from a blank page: start threads, give
each one its own data, collect results, and protect shared data with a
mutex or a read-write lock. You will also be able to explain *why* one way
of locking is faster than another.

Words in **bold** link to the [glossary](../../GLOSSARY.md) the first time
they appear.

---

## 1. The problem

Your laptop has several cores. A normal C program uses only one of them.
Say you want to add up 100 million numbers. With 4 cores, you would like 4
workers, each adding a quarter of the numbers, and then you add the 4
partial sums together.

All the cores of one computer can read and write the **same memory** (L04
slide 2 calls this a *shared-memory system*). So the workers do not need to
send each other messages. They can simply use the same variables. That makes
cooperation easy, but it is also where the trouble starts: two workers
writing the same variable at the same time can lose updates (module 01).

This module answers three questions:
1. How do I start workers and wait for them in C?
2. How do I give each worker its own piece of the work, and get its result back?
3. When workers must update shared data, how do I keep it correct, and fast?

## 2. The idea

A [**thread**](../../GLOSSARY.md#thread) is one sequence of steps inside a
program. A program starts with one thread, the *main thread*. It can start
more threads. All threads of a program share its memory.

**Pthreads** (short for *POSIX threads*) is the standard C library for
threads on Linux and other Unix-like systems (L04 slide 3). POSIX is the
name of the standard that these systems follow.

The basic pattern is called **fork/join** (L04 slide 9):

```
main thread:  ----+------------------------------+----> continues
                  | fork (pthread_create)        | join (pthread_join)
thread 0:         +----- works on part 0 --------+
thread 1:         +----- works on part 1 ----+   |      (finishes earlier;
thread 2:         +----- works on part 2 ---------+      join waits for the last)
```

*Fork* means "start a new thread". *Join* means "wait until that thread
has finished".

**What is shared and what is private?** This decides where races can happen.

| Kind of variable | Shared or private? | Example |
|---|---|---|
| global variable (declared outside every function) | **shared**: one copy for all threads | `double sum;` at the top of the file |
| memory from `malloc` | **shared** by anyone who has the pointer | the matrix `A` in exercise 2 |
| local variable (declared inside a function) | **private**: each thread has its own copy, on its own *stack* | `double my_sum = 0;` inside the thread function |

A *stack* is the area of memory where a function keeps its local variables.
Every thread has its own stack, so two threads running the same function
each get their own local variables.

## 3. The mechanism

### 3.1 Starting and joining threads (L04 slides 4–13)

Here is a complete program, written for this repo:

```c
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>

void *hello(void *arg) {                 // every thread function looks like this
    long id = (long)(intptr_t)arg;       // get our number back out of arg
    printf("hello from thread %ld\n", id);
    return NULL;                         // nothing to return
}

int main(void) {
    pthread_t tid[4];                    // one "handle" per thread
    for (long i = 0; i < 4; i++)
        pthread_create(&tid[i], NULL, hello, (void *)(intptr_t)i);   // fork
    printf("hello from main\n");
    for (int i = 0; i < 4; i++)
        pthread_join(tid[i], NULL);      // join: wait for thread i to finish
    return 0;
}
```

Compile it with `gcc -pthread -o hello hello.c`. The `-pthread` flag tells
the compiler and the linker to use the Pthreads library. (The slides use
`-lpthread`; see section 7.)

**The output order is not fixed.** The four `hello` lines can come out in
any order, and "hello from main" can be first, last or in between (L04
slide 8). The operating system decides when each thread runs. Never write
code that depends on which thread runs first.

`pthread_create(&tid[i], NULL, hello, arg)` takes four things:

| Argument | Meaning |
|---|---|
| `&tid[i]` | where to store the new thread's handle (a `pthread_t`). You need it later to join. |
| `NULL` | thread options (stack size and so on). `NULL` means "use the defaults". |
| `hello` | the function the new thread runs. It must take one `void *` and return a `void *`. |
| `arg` | the single value passed to that function. |

`pthread_join(tid[i], &ret)` waits until thread `i` has finished and, if
`ret` is not `NULL`, stores the thread's return value in `ret`. Call it once
for every thread (slide 13).

**Why join?** When `main` returns, the whole program ends, and every thread
still running is killed in the middle of its work. Join is also how you
know a thread's results are complete and safe to read (section 4).

### 3.2 Giving each thread its data (L04 slide 12)

A thread function gets exactly **one** `void *`. A `void *` is a pointer
that can point to anything. There are two good ways to use it.

**Way 1: a small number inside the pointer value.** Convert the number to
a pointer and back:

```c
pthread_create(&tid[i], NULL, worker, (void *)(intptr_t)i);   // send i
long i = (long)(intptr_t)arg;                                   // receive it
```

`intptr_t` is an integer type that is exactly big enough to hold a pointer,
so no information is lost. The *value* of `i` is copied at the moment you
call `pthread_create`.

**Way 2: a struct per thread.** When a thread needs several values, put
them in a struct and give each thread a pointer to **its own** struct:

```c
struct block_arg { int id, nthreads; long n; long first, last, partial; };
struct block_arg args[P];
for (int t = 0; t < P; t++) {
    args[t].id = t;  args[t].nthreads = P;  args[t].n = n;
    pthread_create(&tid[t], NULL, block_worker, &args[t]);
}
```

The struct can also hold **results** ("out" fields). The thread fills in
`partial`, and `main` reads it after the join. Exercise 1 practises this.

**The classic bug: passing `&i`.**

```c
for (int i = 0; i < 4; i++)
    pthread_create(&tid[i], NULL, worker, &i);    // WRONG: every thread gets the same address
```

All four threads get the address of the *same* variable `i`. A thread reads
`*arg` only when it actually starts running, and by then `main` may have
changed `i`:

| step | main | thread 0 | thread 1 | value of `i` |
|--:|---|---|---|:--:|
| 1 | create thread 0 with `&i` | | | 0 |
| 2 | `i++`, create thread 1 with `&i` | | | 1 |
| 3 | `i++` | | | 2 |
| 4 | | reads `*arg`: **2** | | 2 |
| 5 | | | reads `*arg`: **2** | 2 |

Both threads think they are thread 2, and nobody does parts 0 and 1. After
the loop, `i` is 4, so a late thread may even see 4. Run
`make run M=03 EX=loop_index_bug` to watch it happen. The fix is Way 1 or
Way 2: each thread must get a value or a variable that nobody else changes.

### 3.3 Getting results back

A thread can hand back a result in two ways:
- **return it**: `return (void *)(intptr_t)(i * i);` and read it with
  `pthread_join(tid[i], &ret)`. This is what Lab 01 Ex 1 asks for.
- **write it into its own struct** (or its own slot of an array), and let
  `main` read it after the join.

Never return the address of a *local* variable of the thread. That variable
disappears when the thread function returns, so the pointer would point to
garbage.

### 3.4 Global variables (L04 slide 10)

The slide warns that global variables "can introduce subtle and confusing
bugs". The reason: every thread can see and change a global variable, so
every global that some thread writes is a possible
[**race condition**](../../GLOSSARY.md#race-condition-data-race). Use
globals only for data that really must be shared (like the final sum), and
keep everything else local or in a per-thread struct.

### 3.5 Splitting work into blocks: matrix-vector multiplication (L04 slides 14–17)

To compute `y = A x`, entry `y[i]` is row `i` of `A` times `x`:
`y[i] = A[i][0]*x[0] + A[i][1]*x[1] + ... + A[i][n-1]*x[n-1]`.

Each `y[i]` needs only row `i` and `x`. So we can give each thread a
**block** of rows. With a 6 × 6 matrix and 3 threads (the slide's example):

| thread | rows | writes |
|---|---|---|
| 0 | 0, 1 | `y[0]`, `y[1]` |
| 1 | 2, 3 | `y[2]`, `y[3]` |
| 2 | 4, 5 | `y[4]`, `y[5]` |

The lecture assumes the number of rows `m` is divisible by the number of
threads `p`, and gives thread `t` the rows from `t*(m/p)` to
`(t+1)*(m/p) - 1`. We use a rule that also works when `p` does not divide
`m`:

```
first = t * m / p        last = (t + 1) * m / p        (rows first .. last-1)
```

Multiply first, then divide. With `m = 10` and `p = 3` this gives rows
[0,3), [3,6), [6,10): sizes 3, 3, 4. The blocks touch and never overlap.

**No lock is needed here.** Each thread *writes* only its own entries of `y`.
`A` and `x` are only *read*. Many threads reading the same data at the same
time is always safe; trouble needs at least one writer. Exercise 2 is this
program.

### 3.6 A shared sum: estimating pi (L04 slides 18–20)

A famous series for pi is

```
pi = 4 * (1 - 1/3 + 1/5 - 1/7 + 1/9 - ...)        term k is (-1)^k / (2k + 1)
```

Thread `t` adds up its block of terms. If every thread adds each term
straight into one global `sum`, we have a race. `sum += term` is three
steps (load `sum`, add, store `sum`), and two threads can interleave them:

| step | thread 0 | thread 1 | `sum` in memory |
|--:|---|---|:--:|
| 1 | loads `sum` (1.0) | | 1.0 |
| 2 | | loads `sum` (1.0) | 1.0 |
| 3 | adds its term, stores 0.667 | | 0.667 |
| 4 | | adds its term, stores 1.2 | **1.2** (thread 0's term is lost) |

That is why slide 20 finds that with two threads "different runs give
different answers". Run `make run M=03 EX=pi_race` to see it.

There is a second, smaller trap: the sign. Term `k` has a plus sign when `k`
is even. A block that starts at an odd `k` must start with a minus.

### 3.7 First fix: busy-waiting (L04 slide 21)

The lecture first protects `sum` with a shared variable `flag` that says
whose turn it is. Thread `t` waits in a loop until `flag == t`, adds, then
passes the turn on:

```c
while (flag != my_id) ;          // busy-wait: do nothing until it is my turn
sum += my_part;                  // critical section
flag = (flag + 1) % thread_count;
```

This is [**busy-waiting**](../../GLOSSARY.md#busy-waiting-spinning): the
thread keeps checking the condition, using the CPU the whole time. It is
correct (in the lecture's model), but it has serious costs:

1. **It wastes a core.** A waiting thread does no useful work, but it keeps
   a core 100% busy.
2. **It forces a fixed order.** Thread 2 must wait for threads 0 and 1,
   even if it finished first.
3. **It collapses with more threads than cores.** If the thread whose turn
   it is is not running right now (the operating system paused it), all the
   others spin uselessly until it gets a core back. Slide 26 shows this: the
   busy-wait times grow sharply once there are more threads than cores,
   while the mutex times stay flat. `make run M=03 EX=busywait_cost`
   reproduces the effect on your machine.
4. **In real C it needs care.** The compiler may keep `flag` in a register
   and never see the change. In the exercises we use `LOAD`/`STORE` from
   [`common/atomics.h`](../../common/atomics.h), which make every read and
   write go to memory in program order (module 05, section 6 explains why).

**Where you put the critical section matters a lot.** If the busy-wait is
inside the loop (once per *term*), the threads hand the turn around
millions of times. If each thread first adds its terms into a **local**
variable and busy-waits only **once** at the end, the handing-over happens
only `p` times. The model exercise
[`m1_busywait_sum.il`](exercises/models/m1_busywait_sum.il) lets you check
the turn-taking protocol with the explorer.

### 3.8 Better fix: a mutex (L04 slides 22–25)

A [**mutex**](../../GLOSSARY.md#lock-mutex) (short for *mutual exclusion*)
is a lock. Only one thread at a time can hold it. A thread that calls
`pthread_mutex_lock` while another thread holds the mutex is put to
**sleep** by the operating system: it uses no CPU until the mutex is free.

```c
pthread_mutex_t sum_lock;                     // shared by all threads
pthread_mutex_init(&sum_lock, NULL);          // in main, before creating threads

void *worker(void *arg) {
    double my_sum = 0.0;                      // local: no lock needed
    for (...) my_sum += term;
    pthread_mutex_lock(&sum_lock);            // ---- critical section ----
    sum += my_sum;
    pthread_mutex_unlock(&sum_lock);          // --------------------------
    return NULL;
}

pthread_mutex_destroy(&sum_lock);             // in main, after joining
```

Compared with busy-waiting, a mutex:
- does not waste a core while waiting (the waiter sleeps);
- does not force an order: whoever gets there first goes first;
- keeps working well with more threads than cores.

**Locking every term vs locking once.** Both are correct, but locking
around every single `sum += term` is very slow: the threads spend almost
all their time queueing for the lock. When many threads want the same lock
at the same time, we say the lock has high **contention**. Exercise 3 and
the lab-track exercise 2 measure this. On the machine used to write this
module, locking every increment (Lab 01 Ex 3 Part A) was 50 to 300 times
slower than counting locally and locking once (Part B).

> **Rule of thumb:** do as much work as possible on private data, and make
> the critical section short and rare.

### 3.9 A shared linked list (L04 slides 27–35)

Now a bigger shared data structure: a **sorted linked list** of integers
with three operations:

| Operation | What it does | Changes the list? |
|---|---|---|
| `Member(v)` | is `v` in the list? | no, only reads |
| `Insert(v)` | add `v` in the right place (no duplicates) | yes |
| `Delete(v)` | remove `v` | yes |

All three walk along the list from the start until they reach the first
node whose value is `>= v`. Insert then links a new node in front of it;
Delete unlinks it.

What goes wrong with no locking (slide 35)? Some examples:
- Thread A is reading node 5 (to go on to `5->next`). Thread B deletes node
  5 and frees its memory. Thread A now follows a pointer from freed memory:
  a crash, or worse, garbage.
- Threads A and B both insert between nodes 2 and 7. Both set
  `node2->next` to their own new node. One of the inserts is lost.

The lecture compares three ways to make the list safe.

**Solution 1: one mutex for the whole list** (slides 36–37). Lock it around
every call:

```c
pthread_mutex_lock(&list_mutex);
r = Member(v);
pthread_mutex_unlock(&list_mutex);
```

Simple and correct. But only one thread can use the list at a time, so the
threads take turns: the list operations run one after another (we say
access is *serialized*). If most calls are `Member`, which only read, this
throws away a lot of possible parallelism. If most calls are `Insert` and
`Delete`, this may be the best choice anyway.

**Solution 2: one mutex per node** (slides 38–39). Each node gets its own
mutex, and a thread locks only the nodes it is looking at. This is called
*fine-grained* locking (many small locks), as opposed to *coarse-grained*
(one big lock). The safe way to walk the list is **hand-over-hand
locking**: lock the next node *before* you unlock the current one, like
climbing a rope hand over hand.

```
list:  head -> 2 -> 5 -> 9 -> NULL        Delete(5)

lock head
lock 2,   unlock head          (holding: 2)
lock 5                         (holding: 2, 5)   5 >= 5: found it
2->next = 9                    unlink 5 while holding the node BEFORE it and 5 itself
unlock 5, free 5, unlock 2
```

Because a thread always holds the lock of the node it is standing on, no
other thread can delete that node under its feet. Several threads can work
in different parts of the list at once. The costs (slide 39): the code is
much more complex; every step along the list locks and unlocks a mutex, so
each operation is much slower; and every node needs extra memory for its
mutex.

In exercise 4 the list starts with a dummy node called a **sentinel**: a
node that is always there and holds no real value. Its mutex plays the role
of the lecture's separate lock for the head pointer, and it means insert and
delete never need a special case for "the first node".

### 3.10 Read-write locks (L04 slides 40–43)

Neither solution lets several `Member` calls read the **same** node at the
same time, even though reading together is harmless. A **read-write lock**
fixes that. It has two ways to lock it:

| A thread wants to… | It calls | It gets in when… |
|---|---|---|
| read (`Member`) | `pthread_rwlock_rdlock(&rw)` | no thread holds the lock for **writing**. Other readers may be inside. |
| write (`Insert`, `Delete`) | `pthread_rwlock_wrlock(&rw)` | **nobody** holds the lock, neither readers nor a writer |

Both are released with `pthread_rwlock_unlock(&rw)`. So: many readers
together, **or** one writer alone, never both (slide 41).

**Which is fastest? It depends on the mix of operations** (slide 43).

- With 99.9% `Member` and very few changes, the read-write lock wins
  clearly: almost every call is a read, so the threads really do run
  together. One mutex for the list does not get faster with more threads,
  and one mutex per node is the slowest of all.
- With 80% `Member`, 10% `Insert`, 10% `Delete`, the writers keep stopping
  the readers. The read-write lock is then about as slow as one mutex for
  the list, and neither gets faster with more threads. One mutex per node is
  still slow.

Our own measurement (exercise 4 benchmark, 200,000 operations shared among
the threads, 1000 keys at the start; your numbers will differ):

| 8 threads, time in seconds | one mutex | per node | read-write lock |
|---|--:|--:|--:|
| 99.9% Member | 0.92 | 0.76 | **0.12** |
| 80% Member / 10% Insert / 10% Delete | 1.05 | **0.74** | 1.06 |

The read-write lock wins easily when nearly every call is a read. Per-node
locking was very slow with 1 thread on this machine (1.34 s, against 0.41 s
for one mutex), because of all the extra locking. With 8 threads on 16
cores it beat the single mutex, because it was the only style that let
writers work at the same time. In the lecture's measurements (a different
machine), per-node locking was the slowest in both mixes. The general lesson
holds: per-node locking pays a high price for every step, and the
read-write lock helps only when writes are rare.

## 4. Why it works

- **Join makes results safe to read.** Everything a thread did before it
  finished is visible to the thread that joined it, after `pthread_join`
  returns. That is why `main` may read `args[t].partial` or `y[]` after the
  join without a lock, and why reading it *before* the join is a race.
- **Mutex.** Between `lock` and `unlock`, no other thread can hold the same
  mutex. So the code in between is a [**critical section**](../../GLOSSARY.md#critical-section-cs):
  at most one thread runs it at a time, which gives
  [**mutual exclusion**](../../GLOSSARY.md#mutual-exclusion), and the
  load-add-store of `sum += x` can no longer be interleaved.
- **Busy-wait with a turn variable.** Only the thread whose number equals
  `flag` can pass the loop. `flag` has one value, so only one thread is in
  the critical section. That thread is the only one that changes `flag`, and
  it does so only when it leaves. (The explorer confirms this for 3
  threads: the only possible output of `m1_busywait_sum.il` is `1 3 6`.)
- **Hand-over-hand locking.** (1) A thread holds the lock of the node it is
  standing on, so that node cannot be deleted or changed under it. (2) To
  change `pred->next`, a thread must hold `pred`'s lock, so two threads can
  never change the same link at once. (3) Delete holds the locks of both
  `pred` and the node it removes. Any other thread that wants to reach that
  node must first lock `pred`, so after the unlink nobody can reach it, and
  freeing it is safe. (4) Every thread takes locks in list order (front to
  back), so two threads can never each hold a lock the other is waiting
  for. That means no [**deadlock**](../../GLOSSARY.md#deadlock).
- **Read-write lock.** Readers never change the list, so any number of
  readers at once see a consistent list. A writer is alone, so its changes
  cannot be seen half-done.

## 5. API cheat-sheet

| Call | What it does |
|---|---|
| `#include <pthread.h>`, compile with `gcc -pthread` | use Pthreads |
| `pthread_t tid;` | a handle for one thread |
| `pthread_create(&tid, NULL, fn, arg)` | start a thread running `fn(arg)`; `fn` has type `void *fn(void *)`; returns 0 on success |
| `pthread_join(tid, &ret)` | wait for the thread to finish; its return value goes into `ret` (pass `NULL` if you don't need it) |
| `pthread_self()` | the calling thread's own `pthread_t` |
| `pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;` | a mutex ready to use (for globals) |
| `pthread_mutex_init(&m, NULL)` / `pthread_mutex_destroy(&m)` | set up / clean up a mutex at run time (e.g. one inside each list node) |
| `pthread_mutex_lock(&m)` / `pthread_mutex_unlock(&m)` | take / release the mutex; a thread that must wait sleeps |
| `pthread_rwlock_t rw = PTHREAD_RWLOCK_INITIALIZER;` | a read-write lock (also `pthread_rwlock_init` / `_destroy`) |
| `pthread_rwlock_rdlock(&rw)` / `pthread_rwlock_wrlock(&rw)` | lock for reading (shared) / for writing (alone) |
| `pthread_rwlock_unlock(&rw)` | release either kind |
| `(void *)(intptr_t)i` and `(long)(intptr_t)p` | put a small integer into a `void *` and take it out again (`<stdint.h>`) |

## 6. Common exam mistakes

- Writing the start function as `void *f(void)`. It must take a `void *`
  argument: `void *f(void *arg)`.
- Passing `&i` of the loop counter to every thread. Each thread needs its
  own value or its own variable.
- Forgetting `pthread_join`, then reading results (or returning from `main`)
  while threads are still running.
- Saying "no lock needed" for a shared sum, or "a lock is needed" for the
  matrix-vector product. Ask: **does more than one thread write the same
  variable?** Only then is a lock needed.
- Putting the whole loop, or every single addition, inside the critical
  section. Accumulate locally, lock once.
- Forgetting an `unlock` on some path (for example an early `return`). The
  next thread waits forever (Lab 01 Q3.3).
- Saying a read-write lock is "always faster". It only helps when reads are
  much more common than writes.
- Saying "busy-waiting is wrong". It is *correct*; the problem is that it
  wastes cores and collapses when there are more threads than cores.

## 7. Careful: what the notes say

- **L04 slide 11:** the `pthread_create` prototype shows the start routine
  as `void* (*start_routine)(void)`. That is garbled. The correct type is
  `void *(*start_routine)(void *)`, a function that takes one `void *` and
  returns a `void *`. Slide 12 shows the correct form. (Already in
  [NOTES_ERRATA.md](../../NOTES_ERRATA.md).)
- **L04 slide 13:** `pthread_join` is shown taking a `pthread_t *`. In fact
  it takes the `pthread_t` itself, **by value**: `pthread_join(tid, &ret)`,
  not `pthread_join(&tid, &ret)`. Only `pthread_create` takes a pointer to
  the `pthread_t`, because it has to fill it in.
- **L04 slide 7:** compiles with `-lpthread`. That usually works, but
  `-pthread` is the recommended flag: it also sets the compiler options that
  threaded code needs. The labs use `-pthread`.
- **L04 slide 26:** "n = 108 terms" means 10⁸ (the exponent was lost in the
  export).
- **L04 slide 43:** says "100,000 ops/thread". Our benchmark instead shares
  a fixed total number of operations among the threads, so the speedup
  column means something.
- **L04 slides 14–17:** the matrix-vector code assumes the number of rows
  is divisible by the number of threads. Section 3.5 gives a rule that works
  for any sizes.
- Most code on the Pthreads slides is a picture that did not survive the
  export. All code in this module is written from scratch.

## 8. [EXTRA] Beyond the slides

**Why the list benchmark looks the way it does.** One mutex per node means
that every step along the list does a lock and an unlock, and every one of
these writes to the memory of the mutex. When several cores keep writing
the same memory, the hardware has to pass that memory back and forth
between them, which is slow (module 06 explains this). Readers under a
read-write lock still all write to the single lock word when they lock and
unlock, which is why even the 99.9% case stops speeding up after a few
threads.

**Writers can starve under a read-write lock.** If readers keep arriving,
there may always be at least one reader inside, and a writer can wait for a
very long time. Linux's default read-write lock prefers readers, so this
really happens. Module 08 builds read-write locks that favour writers (Lab
02).

**`rand()` is not safe to call from several threads.** It keeps a hidden
shared state, so two threads calling it at once race on that state. Use
`rand_r(&my_seed)` with a seed that each thread keeps in a local variable.
(Module 04 uses this for Monte Carlo pi.)

**Deadlock in hand-over-hand locking.** It cannot happen, because every
thread locks nodes in the same order (front to back). If one function
walked the list backwards, two threads could each hold one lock and wait
for the other's: a deadlock. Module 10 turns "always lock in the same
order" into a general rule.

## 9. Now do

| # | Exercise | Type | What you practise |
|---|---|---|---|
| 1 | `exercises/concept/ex1_args_struct.c` | warm-up | a struct per thread, the block rule, results in the struct |
| 2 | `exercises/concept/ex2_matvec.c` | core | matrix-vector product by row blocks; when no lock is needed |
| 3 | `exercises/concept/ex3_pi.c` | core | pi with a mutex per term, local sum + mutex, local sum + busy-wait |
| 4 | `exercises/concept/ex4_list_locks.c` | challenge | sorted list with one mutex, per-node locks, read-write lock; benchmark |
| 5 | `exercises/concept/ex5_sum_from_memory.c` | from memory | create/join + struct + mutex-protected sum, from a blank page |
| L1 | `exercises/lab/lab1_create_join.c` | lab track | Lab 01 Ex 1: N threads return the square of their index |
| L2 | `exercises/lab/lab2_mutex_counter.c` | lab track | Lab 01 Ex 3: Part A (global mutex) vs Part B (local accumulation), with the lab's table |
| m1 | `exercises/models/m1_busywait_sum.il` | model | the busy-wait global sum in the explorer |

Module 01's `lab2_mutex_vs_local` covers the same lab exercise (Ex 3) and
also counts how often you lock. Doing both is good practice.

```bash
make test M=03                         # test all exercises
make test M=03 T=ex4_list_locks        # test one exercise
make tsan M=03                         # the same tests under ThreadSanitizer: finds data races
make run  M=03 EX=ex4_list_locks       # the list benchmark tables
make run  M=03 EX=lab2_mutex_counter   # the Lab 01 Ex 3 table
make run  M=03 EX=loop_index_bug       # the &i bug happening
make run  M=03 EX=pi_race              # slide 20: different answers on every run
make run  M=03 EX=busywait_cost        # slide 26: busy-waiting with more threads than cores
```

**ThreadSanitizer** (`make tsan`) is a tool built into gcc and clang. It
watches every memory access while the program runs and reports two threads
touching the same variable without a lock, even when the result happened to
be right. Run it on your solutions: a passing test plus a clean `make tsan`
is much stronger evidence than a passing test alone.

Then do the [quiz](quiz.md) and check yourself with the
[recall sheet](recall.md). Answers are in
[`answers/03-pthreads/`](../../answers/03-pthreads/), but try first.
