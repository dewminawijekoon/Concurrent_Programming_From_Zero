# 13 · MPI: programming with messages

| | |
|---|---|
| **Time** | about 5 hours |
| **You should know first** | module 02 (speedup), module 03 (threads), module 11 helps (splitting work) |
| **Lectures** | L10 *Distributed Memory Programming with MPI* (slides 1–49) |
| **Helps with** | the exam's MPI outline question (about 10 marks, "result on rank 0", signatures given in an appendix), loop-carried dependence + MPI (4 + 6 marks); modules 14 and 16 |

**Goal:** write and outline MPI programs that split work between processes,
send messages, and combine results with collective operations, using every
argument of the MPI calls correctly.

Words in **bold** link to the [glossary](../../GLOSSARY.md) the first time
they appear. There is no lab for this module.

> **Running MPI.** Install OpenMPI (`bash setup/install.sh`, or use the dev
> container). `make test M=13` says SKIP when `mpicc`/`mpiexec` are missing.
> `make run M=13 EX=...` starts 4 processes; set `NP=8` for 8.

---

## 1. The problem

So far all our threads shared one memory. Thread A writes `x`, thread B
reads `x`. That only works inside one computer. A cluster is many
computers joined by a network, and each has its **own** memory
(L10 slide 2). A process on computer 1 simply cannot read a variable on
computer 2.

The only way to share data is to **send a message**: process 1 packs
some numbers, sends them over the network, and process 2 receives them
into its own memory. This is called **distributed-memory** programming.

**MPI** (*Message Passing Interface*) is the standard library for this
(slides 2–3). It works from C, C++ and Fortran, on laptops and on the
biggest supercomputers. It is *explicit*: you decide what goes where. That
makes it hard work ("intellectually demanding", slide 3), but it scales to
thousands of processes.

## 2. The idea

### 2.1 One program, many copies (SPMD)

You write **one** program and start several copies of it:

```bash
mpicc -g -Wall -o hello hello.c       # compile: mpicc is gcc plus the MPI library (slide 5)
mpiexec -n 4 ./hello                   # run 4 copies (processes) of it (slide 6)
```

Each copy is a separate **process** with its own memory. Each gets a
different number called its **rank**: 0, 1, 2, 3. The program asks "what is
my rank?" and uses `if` statements to give the ranks different jobs. This
style is called **SPMD**: *Single Program, Multiple Data* (slide 14).
Very often rank 0 does something special, like printing results.

### 2.2 The first program (slides 4–14)

```c
#include <mpi.h>
#include <stdio.h>

int main(int argc, char **argv) {
    int my_rank, comm_sz;
    MPI_Init(&argc, &argv);                     // set up MPI; before any other MPI call
    MPI_Comm_size(MPI_COMM_WORLD, &comm_sz);    // how many processes are there?
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);    // which one am I?
    printf("process %d of %d\n", my_rank, comm_sz);
    MPI_Finalize();                             // clean up; no MPI calls after this
    return 0;
}
```

- `MPI_Init` must come before every other MPI call, and `MPI_Finalize`
  after the last one (slides 10–11). You may pass `NULL, NULL` instead of
  `&argc, &argv`.
- A **communicator** is a group of processes that can send messages to
  each other (slide 12). `MPI_COMM_WORLD` is the group of *all* processes
  started by `mpiexec`. Ranks are numbered inside a communicator, from 0 to
  size − 1.
- Every MPI name starts with `MPI_` followed by a capital letter (slide 8).

Run it with 4 processes and the 4 lines can come out **in any order**,
different each time. The processes run at the same time, and nobody
decides who prints first. Exercise 1 shows this, and then fixes the order
by sending all lines to rank 0.

## 3. The mechanism

### 3.1 Sending and receiving (slides 15–20)

```c
int MPI_Send(void *buf, int count, MPI_Datatype type, int dest,   int tag, MPI_Comm comm);
int MPI_Recv(void *buf, int count, MPI_Datatype type, int source, int tag, MPI_Comm comm,
             MPI_Status *status);
```

The arguments, one by one:

| Argument | In `MPI_Send` | In `MPI_Recv` |
|---|---|---|
| `buf` | address of the data to send | address where the data should go |
| `count` | how many items to send | the **most** items you can accept (the buffer size) |
| `type` | the type of one item, e.g. `MPI_INT` | must match the sender's type |
| `dest` / `source` | rank to send to | rank to receive from, or `MPI_ANY_SOURCE` |
| `tag` | a number you choose, to label the message | the label you want, or `MPI_ANY_TAG` |
| `comm` | the communicator, usually `MPI_COMM_WORLD` | the same communicator |
| `status` | — | filled with who sent it, the tag, and the size; or pass `MPI_STATUS_IGNORE` |

Example: rank 1 sends 5 doubles to rank 0.

```c
double data[5];
if (my_rank == 1) MPI_Send(data, 5, MPI_DOUBLE, 0, 42, MPI_COMM_WORLD);   // to rank 0, tag 42
if (my_rank == 0) MPI_Recv(data, 5, MPI_DOUBLE, 1, 42, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
```

Note: `count` counts **items**, not bytes. For a C string, send
`strlen(s) + 1` chars so the closing `'\0'` goes too.

**Data types** (slide 16). MPI needs to know the type, because different
computers may store numbers differently.

| MPI type | C type | | MPI type | C type |
|---|---|---|---|---|
| `MPI_CHAR` | `char` | | `MPI_FLOAT` | `float` |
| `MPI_INT` | `int` | | `MPI_DOUBLE` | `double` |
| `MPI_LONG` | `long` | | `MPI_LONG_DOUBLE` | `long double` |
| `MPI_LONG_LONG` | `long long` | | `MPI_UNSIGNED` | `unsigned int` |
| `MPI_BYTE` | raw bytes | | `MPI_UNSIGNED_CHAR` | `unsigned char` |

**Message matching** (slide 18). A receive takes a message only if all
three match: the **communicator** is the same, the **source** is the
sender's rank (or `MPI_ANY_SOURCE`), and the **tag** is the same (or
`MPI_ANY_TAG`). The tag lets you tell apart messages from the same sender,
for example "this one is data to print" vs "this one is data to add"
(slide 15 notes).

**Who sent it, and how much?** (slides 19–20). With `MPI_ANY_SOURCE` and
`MPI_ANY_TAG`, look at the status afterwards:

```c
MPI_Status status;
int buf[100], how_many;
MPI_Recv(buf, 100, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
printf("from rank %d, tag %d\n", status.MPI_SOURCE, status.MPI_TAG);
MPI_Get_count(&status, MPI_INT, &how_many);     // how many ints actually arrived (<= 100)
```

`MPI_ANY_SOURCE` takes messages *in the order they arrive* (slide 17),
which is useful when rank 0 collects results from workers that finish at
different times.

### 3.2 What send and receive promise (slide 21)

- `MPI_Recv` **always waits** (blocks) until a matching message has arrived.
- `MPI_Send` **may or may not wait.** For a small message, MPI usually
  copies it into a hidden buffer and returns at once. For a big message
  (above a size limit that depends on the MPI library), `MPI_Send` waits
  until the receiver has started the matching receive. So never write a
  program that only works if `MPI_Send` returns early.
- **No overtaking:** two messages from the *same* sender to the *same*
  receiver (same communicator) arrive in the order they were sent.
  Messages from *different* senders have no fixed order.

The middle point causes a classic **[deadlock](../../GLOSSARY.md#deadlock)**
(section 8.3).

### 3.3 The trapezoidal rule (slides 22–29)

To estimate the area under a curve f(x) between a and b, cut it into n thin
strips of width h = (b − a) / n and treat each strip as a trapezoid:

```
area ≈ h · [ f(x0)/2 + f(x1) + f(x2) + … + f(x_{n-1}) + f(xn)/2 ],   with xi = a + i·h
```

**Parallel plan** (slide 26):
1. Each process takes a block of neighbouring strips.
2. Each process adds up the area of its own block. No communication.
3. The pieces are added up on rank 0.

With n = 1024 strips and 4 processes, each gets 256 strips. Rank r's
strips start at `local_a = a + r · 256 · h`. (Exercise 4 handles any n, using
the block rule of section 3.6.)

**Step 3 with point-to-point messages** (slides 27–29):

```c
if (my_rank != 0) {
    MPI_Send(&my_area, 1, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD);
} else {
    total = my_area;
    for (int q = 1; q < comm_sz; q++) {
        MPI_Recv(&piece, 1, MPI_DOUBLE, q, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        total += piece;
    }
}
```

This works, but rank 0 does all p − 1 receives and additions **one after
another** while the others sit idle. With 1000 processes that is 999 steps.

### 3.4 Tree-structured global sum (slides 31–32)

Let the processes help each other, in rounds. With 8 processes:

```
           rank:  0    1    2    3    4    5    6    7
start           [ 5]  [2]  [ 1]  [3]  [ 4]  [6]  [ 2]  [1]
round 0:          0 <- 1    2 <- 3    4 <- 5    6 <- 7      (4 messages at the same time)
                [ 7]       [ 4]       [10]       [ 3]
round 1:          0 <------ 2         4 <------- 6          (2 messages)
                [11]                  [13]
round 2:          0 <------------------4                    (1 message)
                [24]                                         total = 5+2+1+3+4+6+2+1 = 24
```

8 processes need **3 rounds** instead of 7 steps; 1000 processes need 10
rounds instead of 999. In general: ⌈log₂ p⌉ rounds, and p − 1 messages in
total. The rule for round s (with d = 2ˢ): a rank that is a multiple of 2d
receives from rank + d; a rank that is d more than a multiple of 2d sends to
rank − d and is then finished. Exercise 5 checks this rule in plain C, and
exercise 6 runs it with real messages.

Slide 32 shows another tree (pairing 0–4, 1–5, … first). Both take the
same number of rounds; which one is faster depends on the network.

### 3.5 Collective communication (slides 33–41)

Global sums are so common that MPI provides them as one call. A call that
**every process of the communicator** takes part in is called a
**collective**.

```c
int MPI_Reduce(void *sendbuf, void *recvbuf, int count, MPI_Datatype type,
               MPI_Op op, int root, MPI_Comm comm);

MPI_Reduce(&my_area, &total, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
```

Each process gives `my_area`. MPI combines them with `op` and puts the
result in `total` **on rank `root` only** (here rank 0). Inside, a good MPI
library uses a tree like section 3.4. The operations (slide 34) are
`MPI_SUM`, `MPI_PROD`, `MPI_MAX`, `MPI_MIN`, the logical and bitwise ones
(`MPI_LAND`, `MPI_BAND`, `MPI_LOR`, `MPI_BOR`, `MPI_LXOR`, `MPI_BXOR`), and
`MPI_MAXLOC`/`MPI_MINLOC` (the largest/smallest value *and* the rank that
has it). With `count > 1`, each array position is combined separately.

**Rules for collectives** (slides 35–36). They are different from
send/receive:

1. **All** processes of the communicator must call the **same** collective.
   One process calling `MPI_Reduce` while another calls `MPI_Recv` is
   wrong: the program hangs or crashes.
2. The arguments must be **compatible**: for example, every process must
   give the same `root`.
3. Collectives have **no tags**. They are matched only by the communicator
   and the **order** in which they are called. If rank 0 calls Reduce A
   then Reduce B, every rank must call them in that order.
4. `recvbuf` only matters on the root, but every process must still pass
   something (it may be `NULL` on the others).

**The other collectives:**

| Call | What happens | Picture (4 ranks) |
|---|---|---|
| `MPI_Reduce(send, recv, count, type, op, root, comm)` | combine everyone's value; result on root only | `a b c d` → root has `a+b+c+d` |
| `MPI_Allreduce(send, recv, count, type, op, comm)` | combine; **everyone** gets the result (slides 37–39) | `a b c d` → all have `a+b+c+d` |
| `MPI_Bcast(buf, count, type, root, comm)` | root's `buf` is copied to everyone (slides 40–41) | root `x` → all have `x` |
| `MPI_Scatter(send, scount, stype, recv, rcount, rtype, root, comm)` | root's array is cut into p pieces of `scount` items; piece r goes to rank r (slide 45) | root `[A B C D]` → rank r has piece r |
| `MPI_Gather(send, scount, stype, recv, rcount, rtype, root, comm)` | the opposite: everyone's piece is put together on root, in rank order (slide 47) | piece r from rank r → root `[A B C D]` |
| `MPI_Allgather(send, scount, stype, recv, rcount, rtype, comm)` | like Gather, but everyone gets the whole array (slide 48) | all have `[A B C D]` |
| `MPI_Barrier(comm)` [EXTRA] | nobody continues until everyone has arrived | |

Two details people get wrong: in Scatter, Gather and Allgather, `scount`
and `rcount` are the number of items **per process**, not the total. And
`MPI_Scatter` as used here needs n to be a multiple of p (each rank gets
exactly n/p items).

**Allreduce as a butterfly** (slide 39). One way to give everyone the sum
is Reduce followed by Bcast: 2⌈log₂ p⌉ rounds. A butterfly does it in
⌈log₂ p⌉ rounds: in round s, every rank swaps its partial sum with the rank
whose number differs in bit s (rank XOR 2ˢ), and both add.

```
rank:      0    1    2    3
start:    [5]  [2]  [1]  [3]
round 0:  0<->1     2<->3          each pair swaps and adds
          [7]  [7]  [4]  [4]
round 1:  0<->2,    1<->3
          [11] [11] [11] [11]      everyone has the total
```

### 3.6 Splitting the data (slides 42–44)

How do we share n elements between p processes? Three common ways
(n = 12, p = 3):

| Partitioning | Rule | Owners of elements 0..11 |
|---|---|---|
| **block** | neighbouring elements together | `0 0 0 0 1 1 1 1 2 2 2 2` |
| **cyclic** | deal like cards: element i → rank i % p | `0 1 2 0 1 2 0 1 2 0 1 2` |
| **block-cyclic** (block size 2) | deal blocks of b like cards | `0 0 1 1 2 2 0 0 1 1 2 2` |

When n is not a multiple of p, give the first n % p ranks one extra
element: with q = n / p and extra = n % p, rank r owns
`count = q + (r < extra ? 1 : 0)` elements starting at
`first = r·q + min(r, extra)`. (Exercise 3.)

**Which one to choose?** Block is simplest and keeps neighbours together.
If the work per element **grows** along the array (for example testing
larger numbers for primality takes longer), block gives the last rank the
most work, and cyclic or block-cyclic spreads it more evenly. Module 16 is
all about this choice.

**Vector sum** (slides 42–48). To compute z = x + y, each rank needs only
its own piece of x and y, so: rank 0 builds the vectors, `MPI_Scatter`s
them, each rank adds its piece, and `MPI_Gather` collects the pieces of z
on rank 0. Exercise 7 does exactly this, plus an `MPI_Bcast` of a scale
factor.

## 4. Why it works

- **No data races.** Processes share no memory, so two processes can never
  write the same variable. All sharing happens through messages, and a
  receive waits until the data is there. The price: every exchange has to
  be written out, and a missing or mismatched message makes the program
  hang.
- **The tree sum is correct** because every partial sum is sent exactly
  once, to a rank that still collects, and rank 0 never sends. So every
  original value is added into rank 0's total exactly once. Exercise 5's
  test checks this for every p from 1 to 40.
- **Why the tree is faster:** the additions in one round happen on
  different processes at the same time. Rank 0 does only ⌈log₂ p⌉
  receives instead of p − 1.
- **Collectives are safe** as long as every process calls the same
  collectives in the same order, because that order is what MPI uses to
  match them.

## 5. Cheat-sheet (signatures as in the MPI standard)

```c
int MPI_Init(int *argc, char ***argv);              int MPI_Finalize(void);
int MPI_Comm_size(MPI_Comm comm, int *size);        int MPI_Comm_rank(MPI_Comm comm, int *rank);
int MPI_Send(const void *buf, int count, MPI_Datatype type, int dest, int tag, MPI_Comm comm);
int MPI_Recv(void *buf, int count, MPI_Datatype type, int source, int tag, MPI_Comm comm, MPI_Status *status);
int MPI_Get_count(const MPI_Status *status, MPI_Datatype type, int *count);
int MPI_Reduce(const void *sendbuf, void *recvbuf, int count, MPI_Datatype type, MPI_Op op, int root, MPI_Comm comm);
int MPI_Allreduce(const void *sendbuf, void *recvbuf, int count, MPI_Datatype type, MPI_Op op, MPI_Comm comm);
int MPI_Bcast(void *buf, int count, MPI_Datatype type, int root, MPI_Comm comm);
int MPI_Scatter(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm);
int MPI_Gather (same arguments as MPI_Scatter);
int MPI_Allgather(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                  void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm);
// [EXTRA]
int MPI_Barrier(MPI_Comm comm);                     double MPI_Wtime(void);
int MPI_Scan(const void *sendbuf, void *recvbuf, int count, MPI_Datatype type, MPI_Op op, MPI_Comm comm);
int MPI_Sendrecv(const void *sendbuf, int sendcount, MPI_Datatype sendtype, int dest, int sendtag,
                 void *recvbuf, int recvcount, MPI_Datatype recvtype, int source, int recvtag,
                 MPI_Comm comm, MPI_Status *status);
int MPI_Isend(const void *buf, int count, MPI_Datatype type, int dest, int tag, MPI_Comm comm, MPI_Request *req);
int MPI_Irecv(void *buf, int count, MPI_Datatype type, int source, int tag, MPI_Comm comm, MPI_Request *req);
int MPI_Wait(MPI_Request *req, MPI_Status *status);
int MPI_Waitall(int n, MPI_Request reqs[], MPI_Status statuses[]);   // MPI_STATUSES_IGNORE is allowed
```

Special values: `MPI_COMM_WORLD`, `MPI_ANY_SOURCE`, `MPI_ANY_TAG`,
`MPI_STATUS_IGNORE`; status fields `status.MPI_SOURCE`, `status.MPI_TAG`.

## 6. Common exam mistakes

- Calling a collective (`MPI_Reduce`, `MPI_Bcast`, …) inside
  `if (my_rank == 0)`. **Every** process must call it.
- Giving collectives a tag, or a `dest`. They have neither; Reduce has a
  `root`.
- Using the result of `MPI_Reduce` on a rank other than the root. Only the
  root gets it; use `MPI_Allreduce` if everyone needs it.
- `count` in bytes instead of items, or the total instead of the
  per-process amount in Scatter/Gather.
- Forgetting `MPI_Init`/`MPI_Finalize`, or calling MPI before `MPI_Init`.
- Assuming that processes can read each other's variables (e.g. rank 0 reads
  `n` from the keyboard and the others "just use it"). It must be sent, for
  example with `MPI_Bcast`.
- Both partners call `MPI_Send` first with a large message: deadlock
  (section 8.3).
- Assuming output lines from different ranks come out in rank order.
- Forgetting the leftover elements when n is not a multiple of p.

## 7. Careful: what the notes say

- **Most function signatures and code on the slides are images** that did
  not survive the export (slides 4, 9–11, 13, 15–20, 24–29, 33–34, 37,
  40, 45–48). Section 5 lists the real signatures. The exam gives them in
  an appendix, but you must know what each argument means.
- **Slide 21** describes the "cutoff": below it `MPI_Send` buffers, above it
  `MPI_Send` blocks. The cutoff size is chosen by the MPI library and can
  differ between machines and versions; never rely on it.
- **Slide 36** says the output argument is "only used on dest_process". In
  the real function that argument is called the **root**, not a
  destination.
- The slides' vector-sum and scatter examples assume **n is a multiple of
  comm_sz**. For other n use the block rule of section 3.6 (or
  `MPI_Scatterv`, [EXTRA]).

## 8. [EXTRA] Beyond the slides

### 8.1 Timing and barriers

`MPI_Wtime()` returns the wall-clock time in seconds as a `double`. To time
a parallel section fairly, line everybody up first:

```c
MPI_Barrier(MPI_COMM_WORLD);          // everyone starts together
double t0 = MPI_Wtime();
... work ...
double mine = MPI_Wtime() - t0, slowest;
MPI_Reduce(&mine, &slowest, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);   // the job takes as long as the slowest rank
```

### 8.2 Ping-pong: what does a message cost?

A message costs roughly *fixed start-up time* + *size / bandwidth*. On one
machine the start-up time is around a microsecond; across a network it is
much more. Exercise 2 prints a table. Lesson: send **fewer, bigger**
messages.

### 8.3 The send/send deadlock and three fixes

Two partners want to swap arrays:

```c
MPI_Send(out, n, MPI_DOUBLE, partner, 0, comm);    // both do this first...
MPI_Recv(in,  n, MPI_DOUBLE, partner, 0, comm, MPI_STATUS_IGNORE);
```

For a small n, MPI buffers both messages and it works. For a large n, each
`MPI_Send` waits until the partner starts its receive. But the partner is
also stuck in its `MPI_Send`. Each waits for the other forever: a deadlock.
(Try `make run M=13 EX=ex9_exchange ARGS="naive 10"`, then
`ARGS="naive 1000000"`.)

| Fix | How | Why it works |
|---|---|---|
| **Order by rank** | even ranks send then receive; odd ranks receive then send | in every pair one side is receiving, so the other side's send can finish |
| **`MPI_Sendrecv`** | one call that sends and receives | MPI handles both directions together, so it cannot block itself |
| **Non-blocking** | `MPI_Isend` + `MPI_Irecv` start the transfers and return at once; `MPI_Wait`/`MPI_Waitall` waits for them | nothing waits until both transfers are started; do not touch the buffers before the wait |

### 8.4 Prefix sums with `MPI_Scan`, and loop-carried dependences

A loop like

```c
a[0] = 0;
for (i = 1; i < n; i++) a[i] = a[i-1] + i;    // step i needs step i-1
```

has a **loop-carried dependence**, so it looks impossible to split. Two
ways out (both appeared in the exam):

1. **Closed form.** Here a[i] = 0 + 1 + 2 + … + i = i(i + 1)/2. Each
   process computes its own elements directly, no communication at all.
2. **Prefix sum (scan).** When there is no formula, each process first does
   its block alone starting from 0, then finds out how much all **lower**
   ranks added up, and shifts its values by that amount. `MPI_Scan` gives
   rank r the combination of the values of ranks 0..r ("inclusive"):

```
block totals:     rank 0: 6    rank 1: 22    rank 2: 38
MPI_Scan (SUM):          6            28            66
minus own total:         0             6            28    <- offset to add to my local values
```

`MPI_Exscan` gives the "exclusive" version (ranks 0..r−1) directly.
Exercise 10 does this for the loop above and checks every value against
the closed form.

## 9. How to answer "outline an MPI solution" (exam, about 10 marks)

The appendix gives you the signatures. Markers look for:

1. **Setup:** `MPI_Init`, `MPI_Comm_size`, `MPI_Comm_rank`; `MPI_Finalize` at
   the end.
2. **Input:** who knows N? If only rank 0 reads it, `MPI_Bcast` it.
3. **Partitioning:** which items each rank does (block or cyclic), with the
   index formula, including the leftover items. Justify the choice
   (load balance!).
4. **Local work:** a loop over my items into a local variable. No
   communication inside the loop.
5. **Combining:** `MPI_Reduce(&local, &total, 1, MPI_DOUBLE, MPI_SUM, 0,
   MPI_COMM_WORLD)`, with every argument right; say that all ranks call it
   and only rank 0 gets the result.
6. **Output:** `if (my_rank == 0) print total`.

Model answers are in the quiz answers (questions 18 and 19).

## 10. Now do

| # | Exercise | Type | What you practise | Runs without MPI? |
|---|---|---|---|---|
| 1 | `exercises/concept/ex1_hello_rank.c` | warm-up | Init/size/rank/Finalize, Send/Recv to rank 0 | no |
| 2 | `exercises/concept/ex2_ping_pong.c` | warm-up | matching sends and receives; message cost | no |
| 3 | `exercises/concept/ex3_partition.c` | core | block / cyclic / block-cyclic index formulas | **yes** |
| 4 | `exercises/concept/ex4_trapezoid.c` | core | trapezoid rule: point-to-point, then `MPI_Reduce` | no |
| 5 | `exercises/concept/ex5_tree_schedule.c` | core | who sends to whom in the tree and butterfly | **yes** |
| 6 | `exercises/concept/ex6_tree_reduce_mpi.c` | core | the tree and butterfly with real messages | no |
| 7 | `exercises/concept/ex7_vector_sum.c` | core | Bcast, Scatter, Gather | no |
| 8 | `exercises/concept/ex8_allreduce_stats.c` | core | Allreduce for mean and standard deviation | no |
| 9 | `exercises/concept/ex9_exchange.c` | core [EXTRA] | the send/send deadlock and three fixes | no |
| 10 | `exercises/concept/ex10_prefix_scan.c` | challenge [EXTRA] | loop-carried dependence with `MPI_Scan` | no |
| 11 | `exercises/concept/ex11_reduce_sum_from_memory.c` | from memory | the exam's "sum over N items, result on rank 0" | no |

```bash
make test M=13                         # MPI tests say SKIP if OpenMPI is missing
make test M=13 T=ex5_tree_schedule     # one exercise
make run  M=13 EX=ex1_hello_rank       # 4 processes
NP=8 make run M=13 EX=ex6_tree_reduce_mpi
make run  M=13 EX=ex3_partition ARGS="10 4"
```

Then do the [quiz](quiz.md) and check yourself with the
[recall sheet](recall.md). Answers are in
[`answers/13-mpi/`](../../answers/13-mpi/), but try first.
