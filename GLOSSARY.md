# Glossary

Plain-English meanings of the words used in this course, with a tiny example
for each. Lessons link here the first time they use a word. The module
number in brackets says where the idea is taught in depth.

### ABA problem
A compare-and-swap succeeds even though the value **changed and then
changed back** in between, so the thread wrongly thinks nothing happened.
*Example:* a pointer goes A → B → A while you were paused. (15)

### Amdahl's law
The best speedup you can get when part of the program must run serially:
speedup = 1 / ((1 − p) + p/n), where p is the parallel fraction and n the
number of processors.
*Example:* 90% parallel on 10 cores gives at most 5.26×, not 10×. (02)

### Asynchrony
A thread can be **delayed at any moment, for any length of time**, and
you can't predict when.
*Example:* a page fault or the scheduler pausing a thread mid-loop. (00)

### Atomic
A step that happens **all at once**. No other thread can see it half-done
or squeeze in between its parts.
*Example:* reading one `int` is atomic. `counter++` is **not**: it is three
steps (read, add one, write back), and another thread can run in between.

### AVX
Intel and AMD **SIMD instructions** that work on 256-bit registers: 8
`float`s or 4 `double`s in one instruction. (12)

### Banker's algorithm
A way to **avoid** deadlock. Grant a request only if, afterwards, there
is still some order in which every process can get everything it might
need and finish (a *safe state*). (10)

### Barrier (thread barrier)
A meeting point: **no thread continues until all n threads have arrived.**
*Example:* every thread must finish step k of a simulation before anyone
starts step k + 1. (07, 08) Not the same as a *memory* barrier; see
memory fence.

### Blocking (progress condition)
A design where a thread that is paused at the wrong moment, for example
while holding a lock, can **stop all the others**. Most lock-based code is
blocking. (15)

### Bottleneck
The **slowest part** of a system. It limits how fast the whole thing can
go.
*Example:* in a pipeline whose stages take 2, 6 and 3 ms, the 6 ms stage is
the bottleneck. (11)

### Bounded buffer
A queue with room for **at most N items**. Producers wait when it is full;
consumers wait when it is empty. (07, 08)

### Bounded waiting
Once a thread has announced it wants the lock, other threads can **overtake
it only a limited number of times**. (06)

### Busy-waiting (spinning)
Waiting by checking a condition again and again in a loop, e.g.
`while (flag[j]) {}`. The thread keeps using the CPU while it waits.

### Cache and cache line
A **cache** is a core's small, fast, private copy of part of memory. It
copies memory in blocks called **cache lines**, usually 64 bytes. (06)

### Cache coherence
The hardware rule that keeps every core's cached copies **in agreement**,
so a read sees the latest write. (06)

### Check-then-act
A bug where a thread checks a condition and then acts on it, but **the
condition changes in between**.
*Example:* two threads both see "1 ticket left", and both buy it. (01)

### Circular wait
Each thread holds one resource and waits for a resource held by the next
thread, all the way round in a circle. It is one of the four conditions for
deadlock.
*Example:* five dining philosophers each holding their left fork and
waiting for their right one. (09, 10)

### Coalescing (GPU memory)
Neighbouring GPU threads read **neighbouring addresses**, so the hardware
can serve them all with one memory transfer. Scattered reads are much
slower. (12)

### Collective (MPI)
An MPI call that **every process in the group must make**, such as
`MPI_Reduce` or `MPI_Bcast`. If one process skips it, the others wait
forever. (13)

### Compare-and-swap (CAS)
One atomic instruction: "if x still equals the value I expect, set it to
the new value and tell me it worked; otherwise change nothing and tell me
it failed." Lock-free code is built on it. (06, 15)

### Condition variable
A named waiting queue used together with a mutex. `wait` sleeps and gives
up the mutex while sleeping. `signal` wakes one waiter, and is **lost if
nobody is waiting**.
*Example:* `not_empty` in a bounded buffer. (08)

### Confidence interval
A range that very probably contains the true average, worked out from
repeated measurements: mean ± z·s/√n.
*Example:* 5 runs with mean 20 s may give 16.7 s – 23.3 s at 95%. (02)

### Critical section (CS)
The part of the code that uses shared data, where **only one thread at a
time** may be.
*Example:* the lines that read and update a shared bank balance.

### Data parallelism
Doing **the same operation on many independent pieces of data**.
*Example:* `c[i] = a[i] + b[i]` for every i. SIMD, GPUs and OpenMP `for`
loops all use it. (04, 12)

### Deadlock
Every thread is **waiting for another one**, so nobody can ever move again.
*Example:* thread A holds lock 1 and waits for lock 2, while thread B holds
lock 2 and waits for lock 1.

### Divergence (GPU)
Threads of one warp take **different branches** of an `if`, so the warp
must run both branches one after the other, and gets slower. (12)

### Doorway
The first part of a lock's `lock()` code, which a thread always finishes
in a few steps **without waiting** for anyone.
*Example:* in the Bakery lock, "raise my flag and take a ticket". (06)

### Efficiency
How much of the processors' time is useful: efficiency = speedup / p.
*Example:* a 3× speedup on 4 cores is 0.75, i.e. 75%. (02)

### Fair scheduler
A scheduler that does not ignore a thread forever. If a thread is ready to
run all the time, it will get a turn eventually. We assume this when we talk
about livelock and starvation; otherwise any thread could "starve" just
because the scheduler never picks it.

### False sharing
Two threads update **different** variables that happen to sit in the
**same cache line**. The cores keep taking the line from each other, which
makes the program much slower even though no data is really shared.
*Example:* `long count[4]`, one counter per thread. (06)

### First-come-first-served (FCFS)
If thread A finishes its doorway before thread B starts its doorway, A
enters the critical section first. The Bakery lock has this property. (06)

### Flynn's taxonomy
Classifies computers by how many instruction streams and data streams
they handle: SISD, SIMD, MISD, MIMD.
*Example:* a multicore CPU is MIMD; a GPU works mostly SIMD-style. (00)

### Fork/join
The main thread starts (forks) worker threads, then waits for (joins) all
of them.
*Example:* `pthread_create` followed by `pthread_join`, or an OpenMP
`parallel` region. (03, 04)

### GPGPU
"General-purpose computing on GPUs": using a graphics card for ordinary
calculations, not just graphics. (12)

### Granularity
How much work each task gets. *Fine-grained* means many small tasks (more
overhead); *coarse-grained* means few big tasks (worse load balance).
*Example:* a merge-sort task for 2 elements is far too fine. (04, 11)

### Grid, block and warp (CUDA)
One kernel launch starts a **grid** of threads. The grid is split into
**blocks**; threads in a block can share fast *shared memory* and wait for
each other with `__syncthreads()`. The hardware runs threads in groups of
32 called **warps**, which execute in lock-step. (12)

### Hand-over-hand locking
Walking a linked list with one lock per node: **lock the next node before
unlocking the current one**, so no other thread can slip in between. (03, 15)

### Happened-before
Event a *happened before* event b if a comes first in the same process, or
a is sending a message that b receives, or there is a chain of such steps
from a to b. (L03, 14)

### Host and device (CUDA)
The **host** is the CPU and its memory; the **device** is the GPU and its
memory. Data must be copied between them (`cudaMemcpy`). (12)

### Interleaving (schedule)
One possible order in which the steps of different threads run.
*Example:* A1 B1 A2 B2 and A1 A2 B1 B2 are two interleavings of threads A
and B with two steps each.

### Invariant
A statement that is true at the start and **stays true after every step**
of the program. Invariants are the main tool for proving that
synchronization code is correct.
*Example:* for a semaphore used as a lock, (threads inside the critical
section) + (semaphore value) = 1 at all times. (07)

### Kernel (CUDA)
A function marked `__global__` that runs on the GPU, executed by many
threads at once. (12)

### Lamport clock
A counter kept by each process to order events without a shared clock:
add 1 at every event, and when a message arrives set the counter to
max(own, received) + 1. (14)

### Latency and throughput
**Latency** is how long one job takes. **Throughput** is how many jobs
finish per second.
*Example:* a pipeline does not make one item faster (latency), but it
finishes more items per second (throughput). (02, 11)

### Lightswitch
A semaphore pattern: the **first** thread of a group to enter locks a shared
semaphore, and the **last** one to leave unlocks it.
*Example:* the first reader locks writers out of the room; the last reader
lets them in again. (07, 09)

### Linearizable
A concurrent object is linearizable if every operation **seems to happen
at one single instant** somewhere between its start and its end. It then
behaves like a normal one-thread object would. (15)

### Livelock
Threads **keep running**, but nobody makes progress.
*Example:* two people in a corridor both step aside, then both step back,
forever.

### Liveness property
"Something good **eventually** happens."
*Example:* "every thread that wants the lock eventually gets it." You
disprove it by showing a run that goes on forever without the good thing.

### Load balancing (static and dynamic)
Giving every thread about the **same amount of work**, so they all finish
together. *Static*: split the work before starting. *Dynamic*: hand out
pieces while running, to whichever thread is free.
*Example:* fixed blocks of numbers vs a shared "next number" counter.
(01, 11)

### Load imbalance
How uneven the work is: the largest amount of work given to one worker
divided by the average. 1.0 means perfectly even.
*Example:* an imbalance of 2.0 means one worker does twice the average and
the others wait for it. (11, 16)

### Lock contention
Many threads want **the same lock at the same time**, so they spend their
time waiting instead of working.
*Example:* locking a global mutex around every single `counter++`. (01, 03)

### Lock (mutex)
An object with `lock()` and `unlock()` that lets only one thread at a time
pass `lock()` until that thread calls `unlock()`. "Mutex" is short for
*mutual exclusion*.

### Lock-free, wait-free, obstruction-free
Three promises about progress without locks:
- **wait-free**: every operation finishes in a bounded number of its own
  steps, no matter what other threads do;
- **lock-free**: *some* thread always finishes its operation, although a
  particular thread may have to retry many times;
- **obstruction-free**: an operation finishes if it gets to run **alone**
  for long enough.
*Example:* a compare-and-swap retry loop for a counter is lock-free but not
wait-free. (15)

### Loop-carried dependence
An iteration of a loop needs a value written by an **earlier iteration**,
so the iterations cannot simply run in parallel.
*Example:* `a[i] = a[i-1] + i`. (04, 11)

### Lost update
One thread's write **overwrites** another thread's write, so an update
disappears.
*Example:* both threads read 5, both write 6; the counter should be 7. (01)

### Lost wake-up
A thread goes to sleep **after** the signal meant for it was already sent,
so it sleeps forever.
*Example:* signalling a condition variable without holding the mutex. (08)

### Manager–worker (master–worker)
One process or thread **hands out work**; the others ask for a new piece
whenever they finish one. Good when pieces take different amounts of
time. (11, 13, 16)

### Memory fence (barrier instruction)
A CPU instruction that says "finish all my earlier memory writes before
doing any later reads or writes". It stops the CPU from reordering memory
operations across it. Not to be confused with a *thread barrier* (see
barrier).

### Monitor
An object that keeps shared data **private** and lets only one thread at a
time run its operations, with condition variables for waiting.
*Example:* a bounded-buffer object with `put` and `take`. (08)

### Multiplex
A semaphore that starts at N, so **at most N threads** can be inside a
section at once.
*Example:* a car park with 5 spaces. (07)

### Mutual exclusion
The rule that two threads are **never in their critical sections at the
same time**.

### Partitioning (block, cyclic, block-cyclic)
Ways to split n items among p workers. With 12 items and 3 workers:
**block** gives `0000 1111 2222`, **cyclic** gives `012 012 012 012`, and
**block-cyclic** (blocks of 2) gives `00 11 22 00 11 22`. (11, 13)

### Pipeline
Work split into **stages connected by queues**. Each stage works on a
different item at the same time, like an assembly line. (11)

### Prefix sum (scan)
Each output is the sum of all inputs up to that point.
*Example:* input `3 1 4` gives `3 4 8`. (04, 13)

### Priority inversion
A **high-priority thread waits for a low-priority one** that does not get
to run.
*Example:* on one core, a high-priority thread spins waiting for a lock that
a low-priority thread holds, so the holder never runs and never releases
it. (06, 08)

### Quorum
A group of processes chosen so that **any two groups overlap** in at least
one process. Used to grant permission without asking everybody. (14)

### Race condition (data race)
The result depends on the exact timing of threads.
*Example:* two threads each do `counter++` once; the result can be 1
instead of 2.

### Rank and communicator (MPI)
A **communicator** is a group of MPI processes that can send messages to
each other (`MPI_COMM_WORLD` is all of them). Each process has a number in
that group, its **rank**, from 0 to size − 1. (13)

### Read-modify-write instruction
One hardware instruction that reads a memory location, computes a new
value, and writes it back **as one atomic step**.
*Examples:* test-and-set, exchange, fetch-and-add, compare-and-swap. (06)

### Read-write lock
A lock that allows **many readers together, or one writer alone**.
*Example:* `pthread_rwlock_rdlock` for lookups, `pthread_rwlock_wrlock` for
inserts. (03)

### Reduction
Combining many values into **one** (sum, max, …). Each thread first combines
its own part privately, then the partial results are combined.
*Example:* OpenMP `reduction(+:sum)`, MPI `MPI_Reduce`. (04, 13)

### Rendezvous
Two threads each **wait for the other** at a meeting point, so neither
continues until both have arrived.
*Example:* `up(i_arrived); down(you_arrived);` in each thread. (07)

### Resource-allocation graph
A drawing of who holds what and who waits for what. An arrow from a
resource to a process means "held by"; an arrow from a process to a
resource means "waiting for". A cycle can mean deadlock. (10)

### Ricart–Agrawala algorithm
A distributed mutual exclusion algorithm. To enter, a process asks every
other process and waits for all n − 1 replies. A process delays its reply
if it is inside, or if its own request is older (smaller timestamp). (14)

### Safe state
A state from which there is **some order** in which every process can get
everything it may still need and finish. The Banker's algorithm only
allows moves that keep the system in a safe state. (10)

### Safety property
"Something bad **never** happens."
*Example:* "two threads are never in the critical section together." You
disprove it by showing one run that reaches the bad situation.

### Semaphore
A shared counter with two atomic operations: `down` (wait while it is 0,
then subtract 1) and `up` (add 1, possibly waking a waiter).
A *binary* semaphore only holds 0 or 1 and is used like a lock; a
*counting* semaphore can hold any value 0 or more and counts resources.
*Example:* a semaphore that starts at 3 lets at most 3 threads in. (07)

### Sentinel
A special value or dummy node. A *sentinel value* means "no more items"
(e.g. `-1` in a producer–consumer queue; also called a "poison pill"). A
*sentinel node* is a dummy first node, so a linked list is never really
empty. (08, 15)

### Sequential consistency
The simple model the lectures use. All threads' steps happen in **one
single order**, and each thread's own steps stay in the order written.
Real CPUs only behave like this if you use atomics or fences (module 05,
section 6).

### SIMD, vector register and lane
"Single instruction, multiple data": one instruction works on several
numbers held side by side in a wide **vector register**; each position is a
**lane**.
*Example:* one AVX instruction adds 8 pairs of floats. (12)

### Speedup
How many times faster a program runs on p processors: speedup = T₁ / Tₚ.
*Example:* 120 s on 1 core and 40 s on 4 cores is a 3× speedup. (02)

### Spin lock
A lock whose waiting threads **busy-wait** (loop) until it is free.
*Example:* a test-and-set lock. (06)

### SPMD (single program, multiple data)
Every thread or process runs **the same program** and uses its own number
(rank) to decide which part of the data to work on. Typical for MPI. (11, 13)

### Starvation
One thread **waits forever** while others keep getting in.
*Example:* a thread that never wins the lock because others always grab it
first.

### Test-and-set
An atomic instruction that **sets a variable to true and returns its old
value**. If the old value was false, you got the lock. (06)

### Thread
An independent sequence of steps inside a program. Threads of the same
program share memory: they can read and write the same variables.

### Thread pool
A **fixed set of worker threads** that take tasks from a shared queue, so
you don't create a new thread for every task. (11)

### Token ring
Processes sit in a logical ring and pass one **token** around; only the
process holding the token may enter the critical section. (14)

### Turnstile
A semaphore pattern `down(t); up(t);`: threads pass through one at a time,
and whoever holds `t` at 0 can stop everybody at that point.
*Example:* letting a waiting writer block new readers. (07, 09)

### Work and span
**Work** (T₁) is the total time of all the steps. **Span** (T∞) is the
length of the longest chain of steps that must happen one after another.
*Parallelism* = work / span is the best speedup any number of processors
can give.
*Example:* P-FIB(4) has work 17 and span 8, so parallelism 2.125. (02)

### Work stealing
Each worker keeps its own queue of tasks. A worker with nothing to do
**takes a task from another worker's queue**. (11)
