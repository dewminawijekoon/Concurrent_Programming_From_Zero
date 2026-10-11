# 15 · Concurrent data structures ★

| | |
|---|---|
| **Time** | about 5 hours |
| **You should know first** | module 02 (Amdahl's law), module 06 (test-and-set, compare-and-swap, spin locks), module 08 (condition variables, lost wake-ups) |
| **Lectures** | L12 *Concurrent Data Structures* (slides 2–16: locking, combining tree, progress conditions, CAS counter; slides 17–64: bounded queue with two locks; slides 65–76: lock-free queue) |
| **Helps with** | Lab 05, all exercises; exam questions like "classify this code as blocking / lock-free / wait-free", "why must the queue use broadcast?", "explain the lagging tail" |

**Goal:** make a data structure safe for many threads *and* fast. You will
compare one big lock, many small locks, and no locks at all, and you will be
able to say exactly what each one promises.

Words in **bold** link to the [glossary](../../GLOSSARY.md) the first time
they appear.

---

## 1. The problem

Many threads share a counter (L12 slide 3). `value++` is really three steps:
read, add one, write. Two threads can both read 7 and both write 8, so one
increment is lost. That is a [**race condition**](../../GLOSSARY.md#race-condition-data-race).

The easy fix is a [**lock**](../../GLOSSARY.md#lock-mutex) around the
increment (slide 4). It is correct, but slide 5 lists what it costs:

- **The code under the lock runs one thread at a time.** By Amdahl's law
  (module 02), even a small part that runs serially limits the speedup.
- **Everyone fights over the lock variable.** Each core keeps pulling it into
  its own cache.
- **One slow thread holds up everyone.** If the thread holding the lock is
  paused (by the operating system, a page fault, …), all others wait.

Here is our run of `lab2_counters` (each thread adds 1 three hundred
thousand times; your times will differ):

| version | 1 thread | 8 threads | correct with 8? |
|---|--:|--:|:-:|
| no lock (`counter++`) | 0.0005 s | 0.0011 s | **no**: 954 895 instead of 2 400 000 |
| mutex | 0.0054 s | 0.2740 s | yes |
| CAS loop | 0.0027 s | 0.1093 s | yes (769 069 CAS attempts failed and were retried) |
| fetch-and-add | 0.0020 s | 0.0405 s | yes |

More threads made **every** correct version *slower* in total time, because
all threads hammer one memory location. Concurrent data structures are about
reducing that fight, and about what happens when a thread is slow.

## 2. The idea

There are two families of designs (L12 slides 6 and 12):

| Design | How it works | Example in this module |
|---|---|---|
| **Coarse-grained locking** | one lock for the whole structure | `coarse_*` list, mutex counter |
| **Fine-grained locking** | one lock per part (per node), so threads in different parts do not wait for each other | hand-over-hand list, two-lock queue |
| **Non-blocking** (no locks) | threads change shared data with atomic instructions such as CAS; a slow thread cannot hold anyone up | CAS counter, Michael–Scott queue |

**Progress conditions** say how much a method promises to finish, even when
other threads get in the way (L12 slides 12–14). From strongest to weakest:

| Condition | Plain meaning | Example |
|---|---|---|
| **Wait-free** | *Every* call finishes within a bounded number of *its own* steps, whatever the other threads do. | `atomic_fetch_add(&c, 1)`: one instruction, always done. |
| **Lock-free** | In any run, *some* call finishes within a bounded number of steps. The system always makes progress, but one unlucky thread may retry forever. | the CAS-loop counter |
| **Obstruction-free** | A call is sure to finish if it runs *alone* (nobody else interfering) for long enough. | "try one CAS, give up if it fails" (Lab 05 snippet D), as an increment |
| **Blocking** | A thread that is paused at the wrong moment (holding a lock) can stop the others from finishing. | anything with a mutex or spin lock |

Every wait-free method is also lock-free, and every lock-free method is also
obstruction-free (slide 14). Stronger guarantees are harder to build. The
first three are called **non-blocking**.

**A quick test:** imagine one thread is frozen forever at the worst possible
moment. If the others can be stuck because of it, the method is *blocking*.
If the others can still finish, it is non-blocking. Then ask whether a
*particular* thread can be made to retry forever (lock-free but not
wait-free).

## 3. The mechanism

### 3.1 A sorted list set: coarse vs fine-grained locking (Lab 05 Ex 1)

The set is a linked list sorted by key with a **head** node (key = −∞), so
every real node has a node before it:

```
head(-inf) -> 3 -> 8 -> 15 -> NULL
```

**Coarse-grained:** lock the global lock, run the serial code, unlock.
Correct and simple, but one thread at a time.

**Fine-grained, "hand-over-hand":** every node has its own lock. A thread
walks down holding the locks of two neighbours, `prev` and `curr`, and
always grabs the **next** lock before it lets go of the previous one, like
climbing a rope hand over hand:

```
lock(head)  lock(3)                      holds head, 3
unlock(head)  lock(8)                    holds 3, 8
unlock(3)  lock(15)                      holds 8, 15    <- insert 10 here: 8 -> 10 -> 15
```

To change the list you need **both** neighbours locked: inserting 10 means
changing `8->next`, and deleting 15 means changing `8->next` too.

**What goes wrong if you release `prev` before locking `curr`** (Lab 05 Q1.2)?
Suppose the walk is "read `curr = prev->next`, unlock `prev`, then lock
`curr`":

| step | thread A: insert(10) | thread B: delete(8) |
|--:|---|---|
| 1 | holds node 3, reads `curr = 8` | |
| 2 | unlocks 3 (holds nothing now) | |
| 3 | | locks 3 and 8, sets `3->next = 15`, frees node 8 |
| 4 | locks node 8 (already removed and freed), moves on: prev = 8, curr = 15 | |
| 5 | sets `8->next = 10`, **but node 8 is no longer in the list** | |

The insert of 10 is lost, and A wrote into freed memory. With hand-over-hand,
B cannot lock 8 while A holds it, and A never holds *nothing*, so the link A
is looking at cannot change under it.

**The surprise in the benchmark.** Our run of `lab1_list_set` (1000 keys,
50% lookups):

| threads | coarse (ops/s) | fine (ops/s) |
|--:|--:|--:|
| 1 | 2 249 381 | 287 576 |
| 4 | 988 072 | 481 116 |
| 8 | 655 173 | 403 629 |

Fine-grained locking is **slower** here. Every step down the list costs a
lock and an unlock, so an operation on a list of 500 nodes takes about 1000
lock operations instead of 2. And threads cannot overtake each other on the
way down: they move along the list like a queue of cars behind a slow truck.
Fine-grained locking pays off only when the work per node is large, or when
threads mostly use different parts of the structure. (Lab 05 Q1.1 wants this
discussion, with your own numbers.)

### 3.2 Four counters (Lab 05 Ex 2; L12 slides 3–16)

**CAS**, compare-and-swap (slide 15), does this in **one**
[**atomic**](../../GLOSSARY.md#atomic) step:

```
CAS(x, expected, new):  if x == expected: x = new, return true
                        else:             return false
```

The counter of slide 16 retries until its CAS succeeds:

```c
long get_and_increment(shared_long *c) {
    long old = LOAD(*c);
    while (!CAS(*c, old, old + 1))   // someone changed c after we read it
        old = LOAD(*c);              // read again and retry
    return old;
}
```

**Why it is lock-free.** A CAS fails only if `c` changed after we read it,
and `c` only changes when *another thread's CAS succeeded*. So every failure
means someone else finished an increment: the system as a whole always makes
progress (slide 16 notes).

**Why it is not wait-free** (Lab 05 Q2.2). Nothing stops the same thread
from losing every time: it reads, another thread's CAS gets in first, it
retries, another gets in first again, and so on. Its number of steps has no
bound. In theory it can **starve**; in practice, under heavy contention, some
threads retry many times (our table: 769 069 failed attempts for 2.4 million
increments). Slide 16 suggests *backoff*: wait a little before retrying. That
lowers contention but does not make the method wait-free.

**Why fetch-and-add is faster** (Lab 05 Q2.3). `atomic_fetch_add` is one
hardware instruction (`lock xadd` on x86). The hardware gives the cache line
to one core at a time, and every attempt succeeds, so there are no retries.
A failed CAS has wasted a read, a CAS and a trip of the cache line between
cores, and then it must try again.

### 3.3 The combining tree (L12 slides 6–11)

A counter is a single memory location, so fine-grained locking cannot split
it. A **combining tree** reduces the fight differently. Threads sit at the
leaves of a binary tree. To increment, a thread climbs towards the root.

- If two threads meet at a node, they **combine**: one (the winner) carries
  the sum of both increments upward; the other (the loser) waits at that
  node.
- The winner that reaches the root adds its total to the counter in one go.
- On the way back down, it hands each waiting loser the value it should
  return (as if their increments had happened one after another).

```
            root: counter += 4
              /            \
        node: 2+2          ...
         /      \
   node: 1+1   node: 1+1
    /   \        /   \
  T0    T1      T2    T3        4 increments, but only ONE update of the counter
```

| Pros (slide 10) | Cons (slides 10–11) |
|---|---|
| increments proceed in parallel until the root | parallelism shrinks near the root |
| with *p* threads, a thread climbs only log₂ *p* levels: O(log *p*) | *p* must be fixed in advance; the tree needs O(*p*) memory |
| effective speedup about *p* / log *p* | a lone thread still climbs all log *p* levels: slow when there is little contention; threads beyond *p* must wait |

### 3.4 A bounded queue with two locks (Lab 05 Ex 3; L12 slides 17–64)

Enqueuers work at the tail, dequeuers at the head (slide 18). So use **two
locks**: `enqLock` for the tail end and `deqLock` for the head end. Then one
enqueue and one dequeue can run at the same time.

The list always starts with a **sentinel** node (a dummy whose value means
nothing; slide 20). `head` points to the sentinel and the first real item
is `head->next`. Dequeuing does not unlink the first real node. It just makes
that node the new sentinel and throws the old sentinel away (slide 38). This
is why enqueuers and dequeuers never touch the same pointer.

```
empty:     head, tail -> [S] -> NULL
two items: head -> [S] -> [a] -> [b] <- tail
after deq: head -> [a] -> [b] <- tail        (a is the new sentinel; a's value was returned)
```

Who knows whether the queue is full or empty? A shared atomic counter
`permits` = free places (slides 25–26). Enqueue takes a permit, dequeue gives
one back:

```
enq(v):  lock enqLock
         while permits == 0: wait(notFull, enqLock)        // full
         tail->next = new node; tail = new node
         old = fetch_add(permits, -1)
         unlock enqLock
         if old == capacity:                              // the queue WAS empty
             lock deqLock; broadcast(notEmpty); unlock deqLock

deq():   lock deqLock
         while permits == capacity: wait(notEmpty, deqLock) // empty
         v = head->next->value; head = head->next          // new sentinel
         old = fetch_add(permits, +1)
         unlock deqLock; free the old sentinel
         if old == 0:                                     // the queue WAS full
             lock enqLock; broadcast(notFull); unlock enqLock
         return v
```

Points that exam questions ask about:

- **Why hold `enqLock` while taking the permit?** So that two enqueuers cannot
  both see the last free place and both add an item (slide 31 notes).
- **Why lock `deqLock` just to broadcast `notEmpty`?** A dequeuer checks
  "empty?" and then goes to sleep, both while holding `deqLock`. If the
  enqueuer could broadcast in between, nobody would be asleep yet, the
  broadcast would be lost, and the dequeuer would then sleep forever.
  Holding `deqLock` makes the broadcast wait until the dequeuer is really
  asleep. The code never holds both locks at once, so the two locks cannot
  deadlock (slide 64 warns about exactly this).
- **Why broadcast and not signal?** (Lab 05 Q3.3; slides 56–61). This queue
  wakes dequeuers only when the queue goes from *empty* to *non-empty*. Now
  look at this run with `signal` (wake one):

  | step | what happens | items | asleep |
  |--:|---|:-:|---|
  | 1 | consumers A and B find the queue empty and go to sleep | 0 | A, B |
  | 2 | producer C adds an item; empty → 1, so it signals: A wakes up | 1 | B |
  | 3 | before A gets the lock, producer D adds an item; 1 → 2, no signal | 2 | B |
  | 4 | A takes one item and leaves | 1 | B |
  | 5 | nobody adds more: **B sleeps forever although an item is waiting** | 1 | B |

  This is a **lost wake-up**. With `broadcast` (wake everybody), step 2
  wakes both A and B, and each one re-checks the queue in its `while` loop.
  Slide 61: always use signalAll/notifyAll (`pthread_cond_broadcast`).
  `test_lab3_bounded_queue` puts consumers to sleep first to provoke this.

**The split counter** (slides 62–64; Lab 05 Q3.2, no code needed). The two
sides share no lock, but they still share `permits`: every single enqueue
*and* dequeue updates that one atomic variable, so it bounces between cores
like the counter in section 1. Notice that the enqueuer only ever
*decreases* it and only cares whether it is zero, while the dequeuer only
*increases* it. So split it into two counters:

- `enqSidePermits`, used only by enqueuers (under `enqLock`), and
- `deqSidePermits`, used only by dequeuers (under `deqLock`).

An enqueuer decreases `enqSidePermits`. Only when it reaches 0 does the
enqueuer take `deqLock` and move all of `deqSidePermits` over to its side.
So the two sides synchronize only now and then, instead of on every call.
Here an enqueuer holds both locks for a moment, so lock order must be fixed
(always `enqLock` first) to avoid deadlock.

### 3.5 The Michael–Scott lock-free queue (L12 slides 65–76)

The same sentinel list, but no locks: everything is done with CAS on
`tail`, `head` and the nodes' `next` fields.

**Enqueue has two steps** (slides 67–71):

1. **Logical enqueue:** `CAS(last->next, NULL, node)`. The moment this
   succeeds, the node *is* in the queue.
2. **Physical enqueue:** `CAS(tail, last, node)`, to move `tail` forward.

```
after step 1:  head -> [S] -> [a] -> [new]        tail still points to [a]: it LAGS
after step 2:  head -> [S] -> [a] -> [new] <- tail
```

The two steps are not one atomic action, so another thread can arrive in
between and find `tail` pointing at the second-to-last node (slide 72). It
can tell, because `tail->next` is not NULL. Instead of waiting for the slow
thread (that would be blocking), it **helps**: it does
`CAS(tail, last, last->next)` itself and then retries its own operation
(slide 73).

```c
void enq(q, v):
    node = new node(v)
    loop:
        last = tail; next = last->next
        if last != tail: continue                 // tail moved while we read: retry
        if next == NULL:
            if CAS(last->next, NULL, node):       // logical enqueue
                CAS(tail, last, node)             // physical; failing is OK
                return
        else:
            CAS(tail, last, next)                 // help the lagging tail
```

**When a CAS fails** (slide 74):

- *Logical step fails:* another enqueuer linked its node first. Start
  again. This is still lock-free, because the failure means that other
  enqueue succeeded.
- *Physical step fails:* someone already moved `tail` for us (a helper).
  Nothing to do: ignore it.

**Dequeue** (slides 75–76 show only pictures; the code is standard
[EXTRA]): read `first = head`, `last = tail`, `next = first->next`.

- If `first == last`: either the queue is empty (`next == NULL`, return
  "empty"), or the tail lags behind an enqueue in progress. Then help move
  `tail` and retry. (Without this, `head` could move past `tail`.)
- Otherwise read `v = next->value` **before** the CAS, then
  `CAS(head, first, next)`. If it succeeds, `next` is the new sentinel and `v`
  is the result. If it fails, another dequeuer took that item: retry.

**Freeing nodes.** In our exercises dequeued nodes are never freed while the
queue is in use (`ms_queue.h` hands nodes out from a pool). Another thread
might have read a pointer to a node a moment ago and be about to read its
`next` field. Freeing it then would make that thread read freed memory. And
if `malloc` reused the address for a new node, a CAS that compares addresses
could succeed when it should fail (the ABA problem, section 8.2).

## 4. Why it works

- **Hand-over-hand list.** To change the link `prev -> curr`, a thread holds
  both locks. To reach a node, any thread must first hold the lock of the
  node before it. So while A holds `prev` and `curr`, nobody else can be
  between them, unlink `curr`, or insert after `prev`. Threads also always
  lock in list order (head first), so no two threads wait for each other in
  a circle: no [**deadlock**](../../GLOSSARY.md#deadlock).
- **Two-lock queue.** *Never more than `capacity` items:* enqueuers check
  and take a permit while holding `enqLock`, so they go one at a time, and
  dequeuers only ever add permits. *A dequeuer never finds `head->next`
  NULL:* the enqueuer links the node *before* it takes the permit (its
  fetch-and-add makes the link visible), and the dequeuer only goes ahead
  when it sees fewer than `capacity` permits.
- **Michael–Scott queue, lock-free.** A thread can loop only if one of its
  CAS operations fails or it finds the tail lagging. A CAS on `last->next`
  or `head` fails only because another thread's CAS on it succeeded, which
  completed that thread's operation. A lagging tail is fixed by one
  successful CAS, after which the next round can make progress. So in any
  run, someone always finishes: lock-free. One thread can still lose every
  round, so it is not wait-free.
- **Invariant of the Michael–Scott queue** (slide 72): `tail` always points
  to the last node or the one just before it, never further back.

## 5. API cheat-sheet

| Lab 05 (GCC builtins) | This repo (`common/atomics.h`, C11) | Meaning |
|---|---|---|
| `__sync_bool_compare_and_swap(&x, old, new)` | `CAS(x, old, new)` | if x == old, set x = new and return true; else false |
| `__sync_fetch_and_add(&x, d)` | `FETCH_ADD(x, d)` | add d, return the OLD value |
| `x` (plain read) | `LOAD(x)` | read a shared atomic variable |
| `x = v` | `STORE(x, v)` | write it |
| | `_Atomic(node *) next;` | an atomic pointer (for CAS on links) |
| `pthread_cond_wait(&cv, &m)` | same | unlock m, sleep, lock m again before returning; always inside `while` |
| `pthread_cond_broadcast(&cv)` | same | wake **every** thread waiting on cv |

## 6. Common exam mistakes

- **Lock-free ≠ "uses no lock variable".** It is a promise about progress.
  A spin lock built from test-and-set has no mutex but is still **blocking**.
- **Lock-free ≠ wait-free.** The CAS loop always lets *someone* finish, but a
  particular thread can retry forever.
- **Fine-grained is not automatically faster.** It costs a lock per node.
- **Hand-over-hand:** take the next lock **before** releasing the previous.
- **`signal` vs `broadcast`:** the "wake only on empty → non-empty" queue
  needs broadcast; with signal it can lose wake-ups (section 3.4 table).
- **Waiting with `if` instead of `while`:** a woken thread may find the
  condition false again (another thread got there first: slides 52–55).
- **Michael–Scott enqueue:** the node is in the queue after the *first* CAS
  (logical), not the second. A failed second CAS is not an error.
- **Classifying with the wrong question.** Ask "if a thread freezes at the
  worst moment, can others finish?", not "is there a loop?".

## 7. Careful: what the notes say

- **No full code for the two-lock bounded queue, and none for the lock-free
  `deq()`.** L12 shows the queue step by step in pictures and gives only the
  class fields (slides 42–43). Section 3.4 and `lab3_bounded_queue` give
  complete code; section 3.5 gives the standard `deq()` [EXTRA]. (This is
  already listed in `NOTES_ERRATA.md`.)
- **Slide 71** (`enq()` of the lock-free queue) is missing the final closing
  brace of the method. The logic is fine.
- **Slide 39** speaker notes say the dequeuer does not need the lock "while
  we decrement the permits"; the dequeuer *increments* permits.
- **Linearizability** (what "correct" means for a concurrent queue) is never
  defined in the notes. Section 8.1 explains it [EXTRA].
- **Slide 15** says any data structure has a wait-free implementation if the
  machine has CAS. That is a true theorem (a "universal construction"), but
  the general construction is far too slow for real use.
- **Lab 05 Ex 3** suggests that the dequeuer waits while `head == tail`, and
  declares `permits` as `volatile int`. In C, reading `tail` in the dequeuer
  while an enqueuer writes it (under a different lock) is a data race, and
  `volatile` does not make `permits--` atomic. Our version makes `permits`
  atomic and uses it for both the "full" and the "empty" test.
- **Lab 05 Ex 4, snippet D** ("try one CAS, give up if it fails"): the lab's
  four snippets match its four classes one to one, so the intended answer is
  **obstruction-free**: the increment is guaranteed to happen only when no
  other thread interferes. As a *method* it always returns within a few
  steps, so you could also argue "wait-free, but not a correct counter".
  State which reading you use.

## 8. [EXTRA] Beyond the notes

### 8.1 Linearizability in plain words

A concurrent queue is **linearizable** if every run could be explained like
this: each operation takes effect at **one instant** somewhere between its
call and its return, and the results are the same as if the operations had
run one at a time in the order of those instants.

```
time ──────────────────────────────────────────────────────>
A:  |---- enq(x) ----|
B:        |-------- enq(y) --------|
C:                                     |--- deq() -> y ---|
```

Is `deq() -> y` correct? `enq(x)` and `enq(y)` overlap in time, so we may
pick their instants in either order. Choose y's instant first: then the
queue holds y, x, and `deq()` returns y. **Allowed.**

If instead `enq(x)` had *returned before* `enq(y)` was called, x must take
effect first, and a `deq()` that returns y (with x still in the queue) would
**not** be linearizable.

For the Michael–Scott queue, the instants are the successful CASes: the
logical-enqueue CAS for `enq`, the CAS on `head` for a successful `deq`, and
the read of `next == NULL` for an empty `deq`. For the locked queues, any
moment while the lock is held works.

### 8.2 The ABA problem

A CAS checks only that a pointer *looks* the same, not that nothing happened
in between. Popping from a lock-free stack (Treiber's stack, the push of
`ex1_cas_warmup`) shows the danger:

| step | thread 1: pop() | thread 2 | stack |
|--:|---|---|---|
| 1 | reads top = A and A->next = B, then is paused | | A → B → C |
| 2 | | pops A | B → C |
| 3 | | pops B (and frees it) | C |
| 4 | | pushes a new node that happens to reuse A's memory | A → C |
| 5 | `CAS(top, A, B)` **succeeds**, because top is A again | | B → ??? |

The stack now starts with B, a node that was removed and freed. The value
went A → B → A ("ABA"), and the CAS could not tell. Common fixes: never reuse
nodes while any thread might hold a pointer (our exercises, `ms_queue.h`),
add a version counter next to the pointer and CAS both together, or use
safe memory reclamation schemes (hazard pointers, epochs).

## 9. Now do

| # | Exercise | Type | What you practise |
|---|---|---|---|
| m1 | `exercises/models/m1_cas_counter.il` | model | the CAS loop in the explorer; compare with "give up after one CAS" |
| 1 | `exercises/concept/ex1_cas_warmup.c` | warm-up | CAS loops: fetch-and-add, atomic max, lock-free stack push |
| 2 | `exercises/concept/ex2_ms_queue.c` | core / challenge | the Michael–Scott queue with C11 atomics, including helping |
| 3 | `exercises/concept/ex3_ms_queue_from_memory.c` | from memory | the same queue without hints |
| L1 | `exercises/lab/lab1_list_set.c` | Lab 05 Ex 1 | coarse vs hand-over-hand list; throughput table |
| L2 | `exercises/lab/lab2_counters.c` | Lab 05 Ex 2 | unsafe, mutex, CAS loop and fetch-and-add counters |
| L3 | `exercises/lab/lab3_bounded_queue.c` | Lab 05 Ex 3 | two-lock bounded queue, permits, broadcast |
| L4 | `exercises/lab/lab4_progress.c` | Lab 05 Ex 4 | classify snippets; snippet D in code |

```bash
make test M=15                               # test all exercises
make test M=15 T=lab3_bounded_queue          # test one exercise
make run  M=15 EX=lab2_counters              # the Lab 05 Ex 2 table
make tsan M=15                               # look for data races
python3 tools/interleave/interleave.py modules/15-concurrent-data-structures/exercises/models/m1_cas_counter.il --outputs
```

Then do the [quiz](quiz.md) and check yourself with the
[recall sheet](recall.md). Answers are in
[`answers/15-concurrent-data-structures/`](../../answers/15-concurrent-data-structures/),
but try first.
