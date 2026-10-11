# 08 · Monitors and condition variables ★

| | |
|---|---|
| **Time** | about 5 hours |
| **You should know first** | module 03 (starting threads, `pthread_mutex_t`), module 05 (critical sections, busy-waiting); module 07 (semaphores) helps but is not required |
| **Lectures** | L09 *Basic Constructs II* (slides 3–28); L12 *Concurrent Data Structures* (slides 44–61: monitor locks, lost wake-up) |
| **Helps with** | **Lab 02, all five exercises** (ping-pong, bounded buffer, many producers/consumers, readers-writers, writer priority); exam questions like "why `while` and not `if`?", "what happens under signal-and-continue?", "monitors vs semaphores: true or false?" |

**Goal:** make a thread *sleep* until something it needs has happened, and
wake it up at the right moment, without wasting CPU and without losing the
wake-up. By the end you can write a bounded buffer and a reader-writer lock
from memory with a mutex and condition variables, and explain every line.

Words in **bold** link to the [glossary](../../GLOSSARY.md) the first time
they appear.

---

## 1. The problem: waiting is hard to do well

Very often a thread cannot go on until something happens. A consumer needs
an item in the buffer. A writer needs all readers to leave. A "pong" thread
needs "ping" to have been printed.

### Waiting by spinning wastes the CPU, and can block for ever

In module 05 we waited with a loop like `while (count == 0) {}`. That is
[**busy-waiting**](../../GLOSSARY.md#busy-waiting-spinning). L09 slide 3
lists two problems:

1. **It wastes CPU time.** The waiting thread runs at full speed, checking
   the same variable millions of times, while doing no useful work. That
   core could have run the thread we are waiting for.
2. **It can stop the program completely.** Take one CPU core and two
   threads. Thread *L* has low priority and is inside a
   [**critical section**](../../GLOSSARY.md#critical-section-cs). Thread *H*
   has high priority and spins, waiting for L to leave. The scheduler always
   runs the highest-priority thread that is ready. H is always ready (it is
   spinning), so L never runs, so L never leaves, so H spins for ever. This
   is the **priority problem** of busy-waiting (textbooks call it *priority
   inversion*).

### The fix: sleep, and be woken

Instead of spinning, the waiting thread should **sleep**: tell the
operating system "don't run me until someone wakes me". The thread that
makes the change then **wakes it up**. Sleeping uses no CPU, and a
sleeping H no longer blocks L. The lecture calls this *sleep and wakeup*.

Semaphores (L08, module 07) already do this: `down()` sleeps when the
counter is 0, and `up()` wakes a sleeper. So why do we need anything else?

### Semaphores work, but are easy to get wrong

L09 slides 7–8 compare semaphores to `goto` and raw pointers: powerful, but
easy to misuse. Typical mistakes:

- forgetting one `up()` or one `down()` (the program hangs, or two threads
  get in together);
- one `up()` too many (now *two* threads can enter a critical section);
- using the wrong semaphore;
- forgetting to protect one of the critical sections.

There are two deeper reasons:

- **A semaphore is global.** To check that it is used correctly you must
  read *every* piece of code that touches it.
- **The same tool does two different jobs.** A semaphore is used both for
  *mutual exclusion* ("one at a time") and for *waiting for an event*
  ("wait until the buffer has an item"). Reading `down(s)` you cannot tell
  which job it does without more context.

A **monitor** separates these two jobs and puts the shared data and the
code that uses it in one place.

## 2. The idea: a monitor

> **Monitor** (L09 slides 9–11): an object (or module) that keeps some
> shared data **private** and offers a few operations on it, with two
> built-in guarantees:
>
> 1. **Mutual exclusion comes for free.** At most one thread is running any
>    of the monitor's operations at a time. Other callers queue at the
>    entrance (the *entry queue*, which the slides call the *service queue*).
> 2. **Condition variables** let a thread inside the monitor sleep until
>    some condition becomes true.

Think of a monitor as a small room with one door and one key. Only the
thread holding the key is inside. The shared data never leaves the room, so
nobody can touch it without the key.

A **condition variable** is a named waiting queue inside the monitor, for
example `notFull` or `notEmpty`. It has these operations (L09 slide 10):

| Operation | What it does |
|---|---|
| `wait(c)` | The calling thread goes to sleep in the queue of `c` **and gives up the monitor** (puts the key back), so other threads can come in. |
| `signal(c)` | If some thread is sleeping in the queue of `c`, wake one of them. If the queue is empty, **nothing happens**: the signal is not remembered. |
| `non_empty(c)` | True if at least one thread is sleeping in the queue of `c`. |
| `broadcast(c)` | (Pthreads, and Java's `notifyAll`) wake **every** thread in the queue. |

Two facts here cause most bugs, so read them twice:

- `wait` **gives up the monitor** while sleeping. Otherwise nobody else
  could get in to make the condition true.
- `signal` **has no memory**. A semaphore's `up()` adds 1 to a counter that
  stays there. A `signal` with nobody waiting is simply lost.

### Example: the bounded buffer as a monitor

A **bounded buffer** is a queue with room for at most N items. Producers put
items in, consumers take them out. A producer must wait while the buffer is
full; a consumer must wait while it is empty. L09 slide 12 writes it as a
monitor. Here is the same idea in our own pseudocode:

```
monitor BoundedBuffer
    private: slots[N], head = 0, tail = 0, count = 0
    condition notFull, notEmpty

    operation put(x):
        if count == N: wait(notFull)        // full: sleep until a slot is free
        slots[tail] = x;  tail = (tail + 1) % N;  count++
        signal(notEmpty)                    // a consumer may be sleeping

    operation take():
        if count == 0: wait(notEmpty)       // empty: sleep until an item arrives
        x = slots[head];  head = (head + 1) % N;  count--
        signal(notFull)                     // a producer may be sleeping
        return x
end
```

Notice what is **missing**: no `lock`/`unlock` anywhere. The monitor makes
`put` and `take` mutually exclusive by itself. The only thing the
programmer writes is *when to wait* and *whom to wake*.

But notice the two `if`s. Whether `if` is good enough depends on *what
happens right after a signal*. That is the next section, and it is the most
examined part of this module.

## 3. The mechanism

### 3.1 Signalling disciplines: who runs after a signal?

L09 slide 13 sets up this situation. Learn it, because exams ask you to
trace it.

1. Thread **A** enters monitor MON and calls `wait(Q)`. A sleeps in Q's
   queue and gives up the monitor.
2. Thread **B** enters MON and starts running monitor code.
3. Thread **C** calls an operation of MON. B is inside, so C waits in the
   entry queue.
4. B calls `signal(Q)`.

Now B is inside, A has just been woken, and C is waiting at the door. Only
one of them may be inside. Who runs next? A **signalling discipline** is
the rule that decides (L09 slides 14–18). There are three:

| Discipline | What B does after `signal(Q)` | Who runs next | Order | Is A's condition still true when A runs? |
|---|---|---|---|---|
| **Signal and exit** | B must leave the monitor at once (the signal is its last action) | A, before C | B → A → C | **Yes.** Nobody ran between the signal and A. |
| **Signal and wait** | B hands the monitor to A straight away and waits itself | A; when A leaves, B carries on; C after both | B (first part) → A → B (rest) → C | **Yes.** A runs immediately after the signal. |
| **Signal and continue** | B keeps running until it finishes its operation | B finishes. Then A (moved to the entry queue) and C compete. **C may go first.** | B → C → A *or* B → A → C | **Not guaranteed.** B may change the data after signalling, and C may "barge in" first and use up what A was waiting for. |

**Java and Pthreads use signal and continue** (L09 slide 23). So in real
code, a woken thread cannot assume its condition is true.

### 3.2 Why `wait` must be inside a `while` loop

Here is the bounded buffer under signal and continue, with **two
consumers** C1, C2 and **one producer** P1, buffer currently empty (L09
slide 24):

| step | what happens | count |
|--:|---|:--:|
| 1 | C1 calls `take()`, sees count == 0, calls `wait(notEmpty)` and sleeps | 0 |
| 2 | P1 calls `put()`, adds an item, calls `signal(notEmpty)`: C1 is woken and joins the entry queue | 1 |
| 3 | P1 leaves the monitor. C2 called `take()` a moment ago and is also at the door. **C2 gets in first** (barging). | 1 |
| 4 | C2 sees count == 1, takes the item, leaves | 0 |
| 5 | C1 finally gets in. It used `if`, so it does **not** check again. It takes from an **empty** buffer. | **−1** |

With `while` instead of `if`, step 5 becomes: C1 checks `count == 0`
again, finds it true, and goes back to sleep. Nothing breaks (L09 slides
17 and 25).

```c
if (count == 0) wait(notEmpty);      // WRONG under signal and continue
while (count == 0) wait(notEmpty);   // RIGHT: check again after every wake-up
```

There are **three** reasons a woken thread may find its condition false.
All three are fixed by the same `while`:

1. **The signaller changed things.** Under signal and continue, B keeps
   running after the signal and may undo the condition.
2. **Another thread barged in** (the C1/C2/P1 example above).
3. **Spurious wake-ups.** POSIX allows `pthread_cond_wait()` to return even
   though nobody signalled. This is rare, but allowed, so code must cope.
   A `broadcast` also wakes threads whose condition may not be true for
   them; to them it looks the same as a spurious wake-up.

Rule for the exam and for life: **always wait in a `while` loop that tests
the condition.**

See it happen for real: `break_it/if_not_while.c` makes the C1/C2 story
happen in every round, and the explorer model `m1_while_not_if.il` finds
exactly the 5-step schedule above by trying every order.

```bash
make run M=08 EX=if_not_while             # with if:    10000 takes from an EMPTY buffer
make run M=08 EX=if_not_while ARGS=while  # with while: 0
```

### 3.3 Monitors in Java (briefly)

L09 slides 19–26. Java has no `monitor` keyword. Instead:

- Every object has a hidden lock. A method marked `synchronized` takes the
  object's lock on entry and gives it back on exit. Make the data `private`
  and every method `synchronized`, and the object behaves like a monitor.
- Each object has **one** unnamed waiting queue (no named conditions like
  `notFull`/`notEmpty`). The operations are `wait()` (sleep and release the
  lock), `notify()` (wake one waiting thread, not necessarily the one that
  waited longest) and `notifyAll()` (wake them all).
- Java uses **signal and continue**, and lets threads barge in.

Because producers and consumers share the single queue, a `notify()` meant
for a consumer could wake a producer. So the Java bounded buffer on slide 26
uses `while (...) wait();` **and** `notifyAll()`. (L12 slides 45–47 show
Java's newer `Lock` + `Condition` objects, which do give named conditions,
like Pthreads.)

### 3.4 Monitors in C: a mutex plus condition variables

C has no monitors built in. We build one by hand from two Pthreads types:

| Monitor idea | Pthreads |
|---|---|
| the monitor's lock (one thread inside) | a `pthread_mutex_t` that **you** lock at the start and unlock at the end of every operation |
| a condition variable | a `pthread_cond_t` |
| `wait(c)` | `pthread_cond_wait(&c, &mutex)` |
| `signal(c)` | `pthread_cond_signal(&c)` |
| signal everyone | `pthread_cond_broadcast(&c)` |

The bounded buffer `put()` from lab track 2:

```c
void put(int item) {
    pthread_mutex_lock(&mutex);                 // enter the monitor
    while (count == size)                       // full? sleep (in a while loop!)
        pthread_cond_wait(&not_full, &mutex);
    buffer[tail] = item;
    tail = (tail + 1) % size;
    count++;
    pthread_cond_signal(&not_empty);            // wake one sleeping consumer, if any
    pthread_mutex_unlock(&mutex);               // leave the monitor
}
```

**What `pthread_cond_wait(&c, &mutex)` does**, step by step:

1. You must already hold `mutex` when you call it.
2. In **one** step that nobody can squeeze into (an
   [**atomic**](../../GLOSSARY.md#atomic) step), it unlocks `mutex` and puts
   the thread to sleep on `c`.
3. When woken (by a signal, a broadcast, or spuriously), it **locks `mutex`
   again** before returning. It may have to wait for the mutex first, and
   other threads can get in during that time. That is exactly why the
   condition may be false again when it returns.

**Signal vs broadcast.** `pthread_cond_signal` wakes *at least one*
waiting thread (usually exactly one). `pthread_cond_broadcast` wakes *all*
of them; they then take turns getting the mutex, and each re-checks its
`while` condition. Use:

- **signal** when *any one* waiter can use the change, and one is enough
  (one item added → one consumer can take it);
- **broadcast** when *several* waiters may now be able to go (a writer
  leaves → all waiting readers may enter), or when waiters on the same
  condition variable wait for *different* things (L12 slide 61's advice:
  "when unsure, wake everyone", because the `while` loops make extra
  wake-ups harmless; they only cost time).

### 3.5 The lost wake-up, and why the mutex prevents it

A **lost wake-up** happens when a thread goes to sleep *after* the signal
it was waiting for has already been sent. Since `signal` has no memory,
nobody will ever wake it. It sleeps for ever, even though the thing it
waits for has happened.

**Way 1: no state variable.** The waiter just calls `pthread_cond_wait`
without checking anything first. If the signal came earlier, it is gone.
*Fix:* keep the condition in a shared variable (`turn`, `count`,
`is_open`) and only wait **while** that variable says "not yet".

**Way 2: changing the state without the mutex.** Suppose the waiter does
everything right, but the signaller does `ready = 1; signal(c);` without
locking the mutex:

| step | waiter (holds the mutex) | signaller (no mutex!) |
|--:|---|---|
| 1 | checks `ready`: it is 0, so it decides to wait | |
| 2 | | sets `ready = 1` |
| 3 | | `signal(c)`: nobody is waiting yet, so the signal is lost |
| 4 | calls `pthread_cond_wait`: sleeps for ever | |

*Fix:* the signaller locks the mutex too. Then step 2 cannot happen
between steps 1 and 4: the waiter holds the mutex from its check until
`pthread_cond_wait` releases it *at the same moment as it starts sleeping*.
By the time the signaller gets the mutex, the waiter is already in the
queue and will hear the signal. **This is why `pthread_cond_wait` takes the
mutex as an argument.**

```bash
make run M=08 EX=lost_wakeup     # nostate: LOST, nolock: LOST, correct: woken up
```

**Way 3: waking too few (L12 slides 56–60).** Two consumers A and B sleep
on an empty queue. A producer adds an item and signals, waking A. Before A
runs, a second producer adds another item but does *not* signal, because
someone wrote an "optimisation": signal only when the queue goes from empty
to non-empty. A takes one item. B sleeps for ever next to an item.
*Fix:* signal on **every** change (or broadcast).

## 4. Why it works

**Mutual exclusion.** Every access to the shared data happens between
`pthread_mutex_lock` and `pthread_mutex_unlock` of the *same* mutex. While
a thread sleeps in `pthread_cond_wait`, it does not hold the mutex and does
not touch the data, so this rule is never broken.

**Safety of the `while` loop.** Look at the line just after the loop:
`while (count == 0) pthread_cond_wait(...); /* here */`. A thread can only
leave the loop by *testing* `count == 0` and finding it false, while
holding the mutex. Nobody else can change `count` before this thread lets
go of the mutex. So at `/* here */` the buffer is definitely not empty.
This argument does not care *why* the thread woke up (signal, broadcast,
barging, spurious), and does not depend on the signalling discipline. That
is what makes the `while` version correct everywhere.

**No lost wake-ups.** Each waiter tests the condition and starts sleeping
as one step as seen by other threads (the mutex is held until
`pthread_cond_wait` atomically releases it). Every thread that makes the
condition true does so holding the mutex, and signals afterwards. So
either the change happened *before* the test (the waiter sees it and does
not sleep), or *after* the waiter was already asleep (the signal finds it).

**Liveness** (no thread sleeps for ever) still needs care: every change
that can make some waiter's condition true must be followed by a signal or
broadcast that reaches a thread that can use it. The tests in this module
check this by making sure every run finishes.

## 5. API cheat-sheet (Pthreads)

```c
pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;   // or pthread_mutex_init(&m, NULL)
pthread_cond_t  c = PTHREAD_COND_INITIALIZER;    // or pthread_cond_init(&c, NULL)

pthread_mutex_lock(&m);   pthread_mutex_unlock(&m);
pthread_cond_wait(&c, &m);       // hold m! releases m + sleeps (atomically); re-locks m before returning
pthread_cond_timedwait(&c, &m, &abs_time);   // same, but gives up at abs_time (returns ETIMEDOUT)
pthread_cond_signal(&c);         // wake at least one waiter (if any); nothing is remembered
pthread_cond_broadcast(&c);      // wake all waiters
pthread_cond_destroy(&c);  pthread_mutex_destroy(&m);

// [EXTRA] ready-made barrier (Linux; not on macOS)
pthread_barrier_t b;
pthread_barrier_init(&b, NULL, n);       // n threads must arrive
pthread_barrier_wait(&b);                // returns PTHREAD_BARRIER_SERIAL_THREAD in one thread, 0 in the others
```

The pattern to know by heart:

```c
// waiting side                          // changing side
pthread_mutex_lock(&m);                  pthread_mutex_lock(&m);
while (!condition)                       make_condition_true();
    pthread_cond_wait(&c, &m);           pthread_cond_signal(&c);   // or broadcast
use_the_shared_state();                  pthread_mutex_unlock(&m);
pthread_mutex_unlock(&m);
```

## 6. Common exam mistakes

- **"`signal` is like `up()`."** No. `up()` is remembered in the counter;
  a `signal` with no waiter is lost. That is why you need a state variable.
- **Using `if` around `wait`.** Under signal and continue (Java, Pthreads)
  that is a bug. Say *why*: the signaller may continue, another thread may
  barge in, and spurious wake-ups are allowed.
- **Saying `if` is "always wrong".** It is correct under **signal and
  exit** and **signal and wait**, because the woken thread runs immediately
  after the signal (that is why L09 slide 12 can use `if`). It is wrong
  under signal and continue.
- **Forgetting that `pthread_cond_wait` releases the mutex.** Without that,
  no other thread could ever make the condition true: deadlock.
- **Calling `pthread_cond_wait` without holding the mutex**, or changing the
  condition without the mutex: this opens the lost wake-up window.
- **"`broadcast` instead of `signal` makes code wrong."** With `while`
  loops, broadcast is never *wrong*, only slower (extra threads wake up,
  re-check, and sleep again). Using `signal` where several threads should
  go *is* wrong (some sleep for ever).
- **"Monitors are more powerful than semaphores."** They have the *same*
  power: each can be built from the other (exercise 2 builds a semaphore
  from a monitor). Monitors are *safer and more structured*, not more
  powerful.
- **"A monitor prevents deadlock."** It gives mutual exclusion
  automatically, but threads can still wait for each other for ever (for
  example a missing signal, or two monitors calling each other).

## 7. Careful: what the notes say

- **L09 slide 10 vs slide 12.** Slide 10 says a signalled process "joins the
  service queue", which is how signal-and-**continue** behaves (the woken
  thread competes with new arrivals). But the bounded buffer on slide 12
  uses `if (count == N) wait(notFull)`, which is only correct under
  signal-and-**exit** or signal-and-**wait**. Read slide 12 as written for
  one of those two disciplines. In Java or Pthreads, always use `while`
  (as slide 26 does).
- **L09 slide 10** says each condition queue is FIFO (first in, first out).
  Pthreads and Java make **no** such promise: `notify()`/`pthread_cond_signal()`
  may wake any waiting thread (slide 21 says this for Java; L12 slide 52 says
  the order differs between implementations). Do not rely on FIFO order.
- **L09 slide 26** (Java producer-consumer) is missing the closing brace of
  the class. Harmless.
- **L12 slide 61** says "always use signalAll/notifyAll". That is the safe
  rule of thumb, not a law: when one condition variable is used for one
  kind of waiter and each change can help exactly one waiter, `signal` is
  correct and cheaper (lab tracks 1–3 use it). When in doubt, broadcast.
- **The term "barrier"** means two different things in this course: a
  *thread barrier* (L09 slide 28, section 8 below: wait until all threads
  arrive) and a *memory barrier/fence* (module 05: a CPU instruction).

## 8. [EXTRA] Building a reusable barrier

L09 slide 28 shows what a **barrier** does: n threads each call `wait`,
and nobody gets past until the last one arrives. The slide gives no code.
Here is how to build one with a mutex and a condition variable.

**The obvious version is broken.**

```c
lock; arrived++;
if (arrived == n) { arrived = 0; broadcast; }
else while (arrived < n) wait;      // BUG
unlock;
```

The last thread resets `arrived` to 0 so the barrier can be used again in
the next round. But the woken threads then re-check `arrived < n`, see 0,
and go back to sleep for ever.

**The fix: a generation counter.** Keep a round number `generation`. Each
thread remembers the round it arrived in, and waits **while the round
number has not changed**:

```c
lock;
my_gen = generation;  arrived++;
if (arrived == n) { arrived = 0; generation++; broadcast; unlock; return 1; }
while (generation == my_gen) wait;  // my round is still going on
unlock; return 0;
```

The last thread both resets `arrived` and moves `generation` on. A woken
thread sees the new generation and leaves, even if fast threads have
already started arriving for the next round. Exercise 4 asks you to write
it; the test runs thousands of rounds and checks that nobody leaves early.

In real code, use `pthread_barrier_t` (section 5). The demo in exercise 4
compares the two.

**Names for the disciplines [EXTRA].** Textbooks name them after their
inventors: signal-and-wait is the *Hoare* monitor, signal-and-continue is
the *Mesa* monitor (from the Mesa language, used by Java and Pthreads),
and signal-and-exit comes from Brinch Hansen's *Concurrent Pascal*.

## 9. Now do

| # | Exercise | Type | What you practise |
|---|---|---|---|
| 1 | `exercises/concept/ex1_gate.c` | warm-up | state variable + `while` + broadcast; a lost wake-up if you forget the state |
| L1 | `exercises/lab/lab1_ping_pong.c` | lab (Lab 02 Ex 1) | strict alternation with one mutex and one condition variable |
| L2 | `exercises/lab/lab2_bounded_buffer.c` | lab (Lab 02 Ex 2) | the bounded-buffer monitor with `not_full`/`not_empty` |
| L3 | `exercises/lab/lab3_mpmc.c` | lab (Lab 02 Ex 3) | many producers and consumers; stopping cleanly with sentinels |
| L4 | `exercises/lab/lab4_rw_readers_pref.c` | lab (Lab 02 Ex 4) | readers-writers, readers' preference |
| L5 | `exercises/lab/lab5_rw_writer_priority.c` | lab (Lab 02 Ex 5) | writer priority with `waiting_writers` |
| 2 | `exercises/concept/ex2_cv_semaphore.c` | core | a counting semaphore built as a monitor |
| 3 | `exercises/concept/ex3_cv_semaphore_from_memory.c` | from memory | the same, without looking |
| 4 | `exercises/concept/ex4_barrier.c` | challenge [EXTRA] | a reusable barrier with a generation counter |
| m1 | `exercises/models/m1_while_not_if.il` | model | find the C1/C2/P1 barging schedule, then fix it |

```bash
make test M=08                              # test all exercises
make test M=08 T=lab2_bounded_buffer        # test one exercise
make run  M=08 EX=lab1_ping_pong ARGS="10"  # run the lab program on its own
make run  M=08 EX=lab3_mpmc                 # configurations A, B, C with a timing table
make run  M=08 EX=lab4_rw_readers_pref ARGS="5 1"   # timestamped log, R=5 readers, W=1 writer
make tsan M=08                              # the same tests under ThreadSanitizer
make run  M=08 EX=if_not_while              # break_it: if instead of while
make run  M=08 EX=lost_wakeup               # break_it: two ways to lose a wake-up
python3 tools/interleave/interleave.py modules/08-monitors-and-condition-variables/exercises/models/m1_while_not_if.il
```

The lab programs print the same kind of logs and tables Lab 02 asks for,
so you can study the lab questions with them (the quiz has all of those
questions). Then do the [quiz](quiz.md) and check yourself with the
[recall sheet](recall.md). Answers are in
[`answers/08-monitors-and-condition-variables/`](../../answers/08-monitors-and-condition-variables/),
but try first.
