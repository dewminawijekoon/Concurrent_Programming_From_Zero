# 00 · The big picture, and how to use this repo

| | |
|---|---|
| **Time** | about 1.5 hours |
| **You should know first** | basic C (functions, arrays, pointers); nothing about threads yet |
| **Lectures** | L01 *Introduction to Concurrent Programming* (slides 2–16); a little of L03 (slides 14–16) |
| **Helps with** | short exam questions: "concurrent vs parallel?", "classify a GPU with Flynn's taxonomy"; a first look at Lab 01 Ex 1 (module 03 goes deeper) |

**Goal:** know *why* this course exists, the basic words (concurrent,
parallel, distributed, asynchrony, Flynn's taxonomy, shared vs distributed
memory), and how to use the tests and tools in this repo. You will also
start your first threads.

Words in **bold** link to the [glossary](../../GLOSSARY.md) the first time
they appear.

---

## 1. The problem: computers stopped getting faster "for free"

**Moore's law** (L01 slide 2) is an observation, not a law of nature: the
number of transistors on a chip doubles about every two years. For decades
this also meant a faster *clock*. The clock is the beat the processor works
to. A 3 GHz clock ticks 3 billion times per second, and each tick lets the
processor do a little more work.

So, for a long time, a program got faster every year without anyone
changing it. You just bought a new computer. L01 calls this "the free ride"
(slides 4–5).

Around 2005 the clock speed stopped rising. A faster clock makes the chip
use much more power and get much hotter, and we could not cool it any more.
The transistor count kept growing, though. Chip makers used the extra
transistors to put **several processors on one chip**. Each processor is
called a **core**. A chip with several cores is a **multicore** chip.

Here is the catch (slides 6–7):

- An old, single-threaded program uses **one** core. On an 8-core laptop it
  leaves 7 cores idle. It does not get faster on a new machine any more.
- To use the other cores, the programmer must split the work into parts
  that run at the same time, and make the parts **cooperate** correctly.
  This is hard. The parts must share data without corrupting it, and
  splitting the work has costs.

That is what this course teaches: how to split work, and how to make the
parts cooperate **correctly** (modules 01, 05–10) and **fast** (modules 02,
04, 11–13).

There are two more reasons to care (slides 13–14):

- **Some problems are too big for one processor.** Weather forecasts and
  animated films need thousands of processor-years of work. Only many
  processors working at once can finish in time.
- **Waiting is wasted time.** A program that waits for the disk or the
  network can let another part of the program run meanwhile.

## 2. The idea: some basic words

### Sequential vs concurrent

A **sequential** program has one [**thread**](../../GLOSSARY.md#thread): one
sequence of steps, done one after another (L01 slide 9). A **concurrent**
program has several threads that share the same memory (slide 10). Each
thread runs its own steps, but they can all read and write the same
variables.

### Concurrent vs parallel vs distributed (L01 slide 11)

| Word | Plain meaning | Everyday example |
|---|---|---|
| **Concurrent** | several tasks are *in progress* at the same time. They may take turns on one core. | one cook preparing three dishes, switching between them |
| **Parallel** | several tasks *really run at the same instant*, on different cores, and work together on one problem | three cooks each making one part of the same meal |
| **Distributed** | separate programs, often on separate machines, cooperate by sending messages | three restaurants that phone each other to share orders |

The key difference: **concurrency is about structure** (the program has
several tasks that are in progress together). **Parallelism is about
execution** (the tasks really run at the same moment). A concurrent program
on a one-core machine is not parallel: the core switches between threads.
The same program on four cores can run in parallel.

L03 (slides 14–15) makes the same point. On one processor, the operating
system switches quickly between threads. L03 calls this *pseudo-concurrent*
execution. On several processors it is *real* concurrent execution. In this
course we always reason about it the same way: as an
[**interleaving**](../../GLOSSARY.md#interleaving-schedule) of the threads'
steps (module 01).

### Asynchrony: you cannot predict timing (L01 slide 12)

**Asynchrony** means a thread can be delayed at any moment, for any length
of time, without warning. Some causes:

| Cause | How long |
|---|---|
| a *cache miss*: the data is not in the core's small fast memory, so it must come from main memory | short (about 100 ns) |
| a *page fault*: the data is on the disk, not in memory | long (milliseconds) |
| the operating system pauses the thread to let another one run | very long (many milliseconds) |

So **a correct concurrent program must never assume** that one thread is
faster than another, or that a step happens "quickly enough". Any thread
can be paused between any two steps. Module 01 shows what goes wrong when
we forget this.

### Flynn's taxonomy (L01 slide 15)

In 1966 Michael Flynn sorted computers by two questions:
- How many **instruction streams** run at once? (One program counter, or many?)
- How many **data streams** are processed at once? (One value at a time, or many?)

S = single, M = multiple, I = instruction, D = data.

| Class | Meaning | Example |
|---|---|---|
| **SISD** | one instruction stream works on one piece of data at a time | an old single-core processor running a normal program |
| **SIMD** | one instruction is applied to many pieces of data at once | a GPU; the vector instructions in your CPU (AVX adds 8 numbers with one instruction) |
| **MISD** | many instruction streams work on the same data | rare. Used for fault tolerance: several computers compute the same thing and must agree (e.g. flight control) |
| **MIMD** | many instruction streams, each on its own data | a multicore CPU running several threads; a cluster of computers |

**How to classify something:** ask "is there one program counter or many?"
and "does one instruction touch one value or many?". A multicore CPU has one
program counter per core, so it is MIMD. Each core may *also* have SIMD
instructions inside it. A GPU runs the same instruction over many data
items, so it is classed as SIMD (module 12 refines this).

### Shared memory vs distributed memory

| | Shared memory | Distributed memory |
|---|---|---|
| **What it is** | all processors see one common memory (L01 slide 3, L04 slide 2) | each processor has its own private memory (L10 slide 2) |
| **How tasks communicate** | by reading and writing the same variables | by sending messages over a network |
| **Typical hardware** | a multicore laptop | a cluster of separate computers |
| **Main danger** | two threads change the same variable at the same time (a race, module 01) | messages are slow; a task may wait for a message that never comes |
| **Tools in this course** | Pthreads (module 03), OpenMP (module 04) | MPI (module 13) |

Inside a large shared-memory machine, some memory may be closer to some
cores than others (L08-TLP slides 6–7). Module 06 covers that.

### What "correct" means (preview)

L01 slides 33–34 split correctness into two kinds. A
[**safety property**](../../GLOSSARY.md#safety-property) says *something bad
never happens* (for example, two pets are never in the pond together). A
[**liveness property**](../../GLOSSARY.md#liveness-property) says
*something good eventually happens* (for example, a pet that wants to drink
eventually gets to the pond). Modules 01 and 05 use these all the time.

## 3. The road map

L01 slide 16 says the course goes "principles first, then practice". This
repo follows the same plan:

| Stage | Modules | Question it answers |
|---|---|---|
| Why it is hard | 01 races, 02 how much speedup | What goes wrong? How much can we gain? |
| Tools to start threads | 03 Pthreads, 04 OpenMP | How do I run code in parallel in C? |
| Correctness from first principles | 05 mutual exclusion, 06 locks and hardware | How do locks work inside? |
| Higher-level tools | 07 semaphores, 08 monitors, 09 classical problems, 10 deadlocks | How do I coordinate many threads safely? |
| Performance and scale | 11 patterns, 12 GPUs, 13 MPI, 15 data structures | How do I make it fast and big? |

## 4. How to use this repo

Every module has the same parts: a lesson (`README.md`), exercises with
`TODO`s, self-tests, a quiz, a recall sheet, and answers in `answers/`.

```bash
make check                          # which tools does this machine have?
make test M=00                      # test all exercises of module 00
make test M=00 T=ex1_hello_threads  # test one exercise
make run  M=00 EX=ex1_hello_threads ARGS=8    # build and run it on its own
make tsan M=01                      # run the tests under ThreadSanitizer (finds data races)
python3 tools/interleave/interleave.py tools/interleave/programs/counter_race.il --outputs
```

What the test results mean:

| Result | Meaning |
|---|---|
| **PASS** | every check passed. For a concurrency test this means "no problem was *seen* in N runs". It does **not** prove the code correct. |
| **FAIL** | a check failed, or the program hung and the *watchdog* stopped it. A watchdog is a timer that kills a test that takes too long. |
| **SKIP** | the machine lacks something the test needs (for example MPI or a GPU). |

Why a passing test proves little: a bug in concurrent code may need one
rare order of steps. A test might never hit it. Two tools help:

- **ThreadSanitizer** (`make tsan M=..`) watches memory accesses while the
  program runs. It reports two threads that touch the same variable without
  coordination, even if the result happened to be right.
- **The interleaving explorer** (`tools/interleave`) tries **every** possible
  order of steps of a small model. Its answers are certain. Module 01 uses
  it a lot.

**A good study loop for each module:** read the lesson → answer the
"PREDICT FIRST" question in each exercise → write the code → `make test` →
do the quiz on paper → check the answers → cover the recall sheet and say
the answers out loud.

## 5. API cheat-sheet (just enough for exercise 1)

| Call | What it does |
|---|---|
| `pthread_create(&t, NULL, fn, arg)` | start a new thread that runs `fn(arg)`. Returns 0 on success. |
| `pthread_join(t, &result)` | wait until thread `t` finishes, and collect the `void *` it returned |
| `pthread_self()` | the id of the calling thread |
| `sysconf(_SC_NPROCESSORS_ONLN)` | how many cores the machine has right now |

The thread function must look like `void *fn(void *arg)`. To pass a small
integer, cast it: `(void *)(long)i` when creating, `(int)(long)arg` inside.
Module 03 explains these calls fully.

## 6. Common exam mistakes

- "Concurrent" and "parallel" are not the same. A concurrent program can run
  on one core (taking turns). Parallel means really at the same instant.
- A multicore CPU is **MIMD**, not SIMD. The SIMD instructions inside each
  core do not change the class of the whole chip.
- Moore's law is about **transistor count**, not clock speed. Clock speed
  stopped rising; transistor count did not.
- "More cores" does not make an old single-threaded program faster.
- Distributed memory means **no shared variables**: tasks must send
  messages.

## 7. Careful: what the notes say

- **L01 slide 11** defines the three words very briefly. The "structure vs
  execution" way of telling concurrent from parallel is our explanation. It
  is the usual one in textbooks and agrees with L03 slides 14–15.
- **L01 slide 3** (speaker notes) mentions Flynn's classes and "fault
  tolerant schemes" under the shared-memory picture. Flynn's taxonomy is a
  separate idea from shared vs distributed memory: one is about instruction
  and data *streams*, the other about where the *memory* is.
- **L01 slide 17** writes "1010" and "109". The exponents were lost in the
  export: they mean 10¹⁰ and 10⁹ (see [NOTES_ERRATA.md](../../NOTES_ERRATA.md)).

## 8. Now do

| # | Exercise | Type | What you practise |
|---|---|---|---|
| 1 | `exercises/concept/ex1_hello_threads.c` | warm-up | start N threads, pass each its number, collect a result from each (a preview of Lab 01 Ex 1) |
| 2 | `exercises/concept/ex2_split_the_work.c` | core | split a range evenly between threads (L01 slide 18), count primes, and see why an even split is not always fair |

```bash
make test M=00
make run  M=00 EX=ex1_hello_threads ARGS=8
make run  M=00 EX=ex2_split_the_work ARGS="2000000 4"
```

Then do the [quiz](quiz.md) and check yourself with the
[recall sheet](recall.md). Answers are in
[`answers/00-big-picture/`](../../answers/00-big-picture/), but try first.
