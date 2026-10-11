# 14 · Distributed mutual exclusion [EXTRA]

> **This whole module is [EXTRA].** The course syllabus lists distributed
> mutual exclusion for week 12, but **no lecture slides cover it** (see
> [NOTES_ERRATA.md](../../NOTES_ERRATA.md)). Everything here is standard
> textbook material, explained in our own words. It links to what the
> slides do cover: mutual exclusion (L05, module 05), the happens-before
> order (L03 slides 28–29), and message passing (L10, module 13).

| | |
|---|---|
| **Time** | about 3 hours |
| **You should know first** | module 05 (what a correct lock must guarantee), module 13 (MPI send/receive) |
| **Lectures** | none (syllabus week 12); related: L03 slides 28–29, L05 slides 3–4, L10 |
| **Helps with** | design questions about locking without shared memory; module 16 |

**Goal:** explain and compare four ways to get mutual exclusion when
processes can only send messages, and implement two of them.

Words in **bold** link to the [glossary](../../GLOSSARY.md) the first time
they appear.

---

## 1. The problem

Module 05 built locks from shared variables: Peterson's lock reads the
other thread's `flag`. Module 06 used hardware instructions like
test-and-set on a shared memory word. All of these need **shared
memory**.

Now imagine 5 computers that share a printer (or a file, or a database
row). They have no common memory. The only thing they can do is send
messages over a network, as in MPI (module 13). There is no `flag[j]` to
read: by the time a message about it arrives, it may already be out of
date. And messages take time and may arrive in any order.

So we need a new kind of algorithm: **distributed mutual exclusion**. Each
process still repeats

```
non-critical section → entry protocol → critical section → exit protocol
```

but the entry and exit protocols now consist of sending and receiving
messages.

## 2. The idea: what a solution must guarantee

The requirements are the ones from module 05, plus one about order:

| Requirement | Plain meaning | Kind |
|---|---|---|
| **Safety** | at most one process in the critical section at any time | [safety](../../GLOSSARY.md#safety-property) |
| **Liveness** | every request is eventually granted: no [deadlock](../../GLOSSARY.md#deadlock), no [starvation](../../GLOSSARY.md#starvation) | [liveness](../../GLOSSARY.md#liveness-property) |
| **Ordering (fairness)** | if request A *happened before* request B, A is granted first | fairness |

We usually assume that messages are never lost (but may be slow), and that
processes do not crash. Section 3.5 asks what happens when they do.

Because messages are slow, we measure algorithms by:

- **messages per entry**: how many messages one use of the critical section costs;
- **client delay**: how long a process waits to enter when nobody else is
  inside, counted in message times;
- **synchronization delay**: how long the critical section stays empty
  between one process leaving and the next one entering, while someone is
  waiting.

## 3. The mechanism

### 3.1 Lamport logical clocks

Distributed algorithms need to agree on "who asked first". Real clocks on
different computers never agree exactly, so we use **logical clocks**
instead.

**Happened-before** (L03 slides 28–29 call it "precedes", written A → B,
an idea by Leslie Lamport). Event a *happened before* event b if:

1. a and b are in the same process, and a comes first; or
2. a is the sending of a message and b is its receipt; or
3. there is a chain of such steps from a to b.

If neither a → b nor b → a, the events are **concurrent**: nothing could
have told one about the other.

A **Lamport clock** is an integer counter in each process with three rules:

| Event | Rule |
|---|---|
| local event | `clock = clock + 1` |
| send a message | `clock = clock + 1`, and the message carries this value |
| receive a message stamped t | `clock = max(clock, t) + 1` |

Example with three processes (P0 sends m1 to P1, P1 sends m2 to P2, P2
sends m3 back to P0). The number is the event's timestamp:

```
P0:  a(1) ── b: send m1 (2) ─────────────────────────────────── j: recv m3 (7)
                     \                                           /
P1:  c(1) ──────── d: recv m1 (3) ── e: send m2 (4)             /
                                          \                    /
P2:  f(1) ── g(2) ──────────────────── h: recv m2 (5) ── i: send m3 (6)
```

When P1 receives m1 (stamped 2) its clock is 1, so it jumps to
max(1, 2) + 1 = 3. That jump is the whole point: the receive must come
"after" the send.

**What the timestamps promise:** if a → b, then clock(a) < clock(b).
**What they do not promise:** the reverse. g has timestamp 2 and d has 3,
but g and d are concurrent (no chain of messages connects them).

**A total order.** Two events can have the same timestamp (a, c and f all
have 1). Break ties by process id: (t, i) comes before (t', j) if t < t',
or t = t' and i < j. Now every pair of events, and so every pair of
requests, is ordered, and every process computes the same order.
Exercise 1 implements these rules.

### 3.2 Centralized coordinator

The simplest idea: pick one process as the **coordinator**. It keeps the
"lock" and a queue.

```
P2 → coordinator:  REQUEST
coordinator → P2:  GRANT       (if nobody is inside; otherwise P2 goes into the queue)
P2 uses the critical section
P2 → coordinator:  RELEASE     (the coordinator now GRANTs the first process in the queue)
```

- **3 messages per entry**; client delay 2 message times (request + grant);
  synchronization delay 2 (release + grant).
- Safe, because the coordinator grants to one process at a time; fair if
  the queue is first-come first-served.
- Weakness: the coordinator is a **single point of failure** and a
  **bottleneck**: every request goes through it.

### 3.3 Token ring

Arrange the processes in a logical ring, 0 → 1 → … → n−1 → 0. One special
message, the **token**, travels around the ring. A process may enter the
critical section only while it holds the token.

```
token arrives at P:
    if P wants the critical section: use it, then pass the token on
    else: pass the token on straight away
```

- Safe for a simple reason: there is **only one token**.
- No starvation: the token visits every process once per round, so a
  waiting process waits at most n − 1 passes.
- **Messages per entry: from 1 to unbounded**: the token keeps circulating
  even when nobody wants it. Client delay 0 to n − 1 passes.
- Weaknesses: if the token is lost (or the process holding it crashes)
  someone must notice and create exactly one new one; a crashed process
  breaks the ring.

Exercise 4 builds a token ring from threads that share nothing but
mailboxes; exercise 5 does it with MPI processes.

### 3.4 Ricart–Agrawala

No coordinator and no token: a process asks **everybody** and enters once
**everybody has said yes**.

Each process has a state: `RELEASED` (does not want it), `WANTED`
(waiting), or `HELD` (inside).

```
to enter:
    state = WANTED
    T = my Lamport timestamp (tick the clock)
    send REQUEST(T, my id) to all n−1 others
    wait until n−1 REPLYs have arrived
    state = HELD

on receiving REQUEST(T', j):
    if state == HELD, or (state == WANTED and (T, me) is earlier than (T', j)):
        defer: remember j, reply later
    else:
        send REPLY to j now

to leave:
    state = RELEASED
    send REPLY to every deferred process
```

"Earlier" uses the total order of section 3.1: smaller timestamp, and on
a tie the smaller id. So when two processes ask at the same time, every
process agrees on which request came first.

**Example** with 3 processes, all clocks at 0. P1 and P2 ask at the same
moment.

| step | what happens | why |
|--:|---|---|
| 1 | P1: clock 1, sends REQUEST(1, P1) to P0 and P2 | |
| 2 | P2: clock 1, sends REQUEST(1, P2) to P0 and P1 | |
| 3 | P0 replies to both | P0 is RELEASED |
| 4 | P2 receives REQUEST(1, P1) and **replies** | P2 is WANTED, but (1, P1) is earlier than its own (1, P2) |
| 5 | P1 receives REQUEST(1, P2) and **defers** it | P1's own (1, P1) is earlier |
| 6 | P1 has 2 replies (P0, P2): **enters** | |
| 7 | P1 leaves and sends the deferred REPLY to P2 | |
| 8 | P2 has 2 replies (P0, P1): **enters** | |

Each entry cost n − 1 requests + n − 1 replies = **2(n − 1) messages**
(here 2 × 2 = 4 per entry, 8 in total). Client delay: 2 message times (one
round trip). Synchronization delay: 1 message time (the deferred reply).

Exercise 2 writes the "reply or defer" decision, exercise 3 runs N nodes
through thousands of random message orders, and exercise 6 uses real MPI
processes.

### 3.5 Comparison

| Algorithm | Messages per entry | Client delay (no competition) | Sync delay | Main problem |
|---|---|---|---|---|
| Centralized coordinator | 3 | 2 | 2 | coordinator crash = everything stops; bottleneck |
| Token ring | 1 to unbounded | 0 to n − 1 | 1 to n − 1 | lost token; one crash breaks the ring |
| Ricart–Agrawala | 2(n − 1) | 2 | 1 | **any** crash blocks everyone (they wait for its reply) |
| Maekawa (section 7) | about 3√n to 6√n | 2 | 2 | can deadlock unless extra messages are added |

There is no single winner. The coordinator is cheapest in messages; the
token ring is cheap when the critical section is used all the time;
Ricart–Agrawala has no special process but costs the most messages and is
the most fragile with crashes.

## 4. Why it works

**Token ring: safety.** At every moment the token is either inside one
process or in one message on its way. A process enters only while it holds
the token, so at most one process is inside.

**Ricart–Agrawala: safety.** Suppose, to get a contradiction, that Pi and
Pj are both inside. Each got a REPLY from the other. Name them so that
Pi's request (Ti, i) is earlier than Pj's request (Tj, j). Look at the
moment Pj's request arrived at Pi:

- If Pi had already sent its own request, Pi was WANTED or HELD with an
  earlier request, so it **deferred** Pj. It only replies after leaving
  the critical section. So Pj cannot be inside while Pi is.
- If Pi had not yet sent its request, Pi replied. But receiving Pj's
  request moved Pi's clock past Tj, so Pi's later request got a timestamp
  larger than Tj. Then (Ti, i) is *later* than (Tj, j). Contradiction.

Both cases are impossible, so two processes are never inside together.

**Ricart–Agrawala: no deadlock, no starvation.** All requests are totally
ordered. The earliest waiting request gets a reply from everyone: processes
that are RELEASED reply at once, processes that are WANTED have later
requests and reply at once, and a process that is HELD replies when it
leaves. So the earliest request is always granted. After a process enters,
its next request gets a timestamp larger than every request it has seen,
so it goes behind the others. Every waiting request therefore becomes the
earliest one eventually.

**Why the test cannot replace the proof.** Exercise 3's test tries
thousands of random message orders and finds no violation. That is
evidence, not a proof. The argument above covers *every* order.

## 5. Cheat-sheet

```
Lamport clock:   local: c++     send: c++, stamp c     receive(t): c = max(c, t) + 1
Total order:     (t, i) < (t', j)  if  t < t'  or  (t == t' and i < j)

Ricart–Agrawala, on REQUEST(t', j):
    defer if  state == HELD  or  (state == WANTED and (my_t, me) < (t', j))
    else reply now
  enter after n−1 replies; on exit reply to all deferred.
```

## 6. Common exam mistakes

- Using real wall-clock times to decide "who asked first". Computer clocks
  are never exactly in step; use Lamport timestamps.
- Forgetting the tie-break by process id. Two requests can have the same
  timestamp; without a tie-break both may defer (deadlock) or both reply
  (both enter).
- In Ricart–Agrawala, replying to everyone while HELD. A HELD process
  must defer every request.
- Saying the token ring costs "1 message per entry". It costs at least 1,
  and the token keeps moving when nobody wants it.
- Claiming the timestamps show which events are concurrent. Lamport clocks
  only give "a → b implies clock(a) < clock(b)", not the reverse.
- In a real implementation, letting a process stop as soon as it has
  finished its own entries. Others may still need its reply (exercise 6).

## 7. [EXTRA] Maekawa's algorithm, briefly

Ricart–Agrawala asks everybody. Maekawa asks only a **quorum**: a subset
of the processes, chosen so that **every two quorums share at least one
process**. A process enters after every member of its quorum has said
yes, and each member says yes to only one process at a time. The shared
member of two quorums can't say yes to both, so two processes are never
inside together.

A simple choice: put n processes in a √n × √n grid; a process's quorum is
its row plus its column (2√n − 1 processes, including itself). Any
row-plus-column meets any other row-plus-column. Each quorum member gets a
request, sends a grant, and later gets a release: about 3 × 2√n = 6√n
messages per entry. Maekawa showed that quorums of only about √n
processes are possible, giving about 3√n messages. Either way it grows
much more slowly than Ricart–Agrawala's 2(n − 1): for n = 100 the grid
costs about 3 × 18 = 54 messages instead of 198.

The catch: the basic version can **deadlock**. P1 gets the "yes" of the
shared member A, P2 gets the "yes" of the shared member B, and each waits
for the other's member forever. Real versions add extra messages to break
such cycles.

## 8. Now do

| # | Exercise | Type | What you practise | Runs here without MPI? |
|---|---|---|---|---|
| 1 | `exercises/concept/ex1_lamport_clock.c` | warm-up | the three clock rules and the total order | yes |
| 2 | `exercises/concept/ex2_ra_decision.c` | core | Ricart–Agrawala's "reply or defer" decision | yes |
| 3 | `exercises/concept/ex3_ra_simulation.c` | core | the whole algorithm with N simulated nodes and random message order | yes |
| 4 | `exercises/concept/ex4_token_ring.c` | core | a token ring of threads that share only mailboxes | yes |
| 5 | `exercises/concept/ex5_token_ring_mpi.c` | core | the token ring with MPI processes | needs MPI |
| 6 | `exercises/concept/ex6_ra_mpi.c` | challenge | Ricart–Agrawala with MPI, including clean shutdown | needs MPI |

```bash
make test M=14                        # MPI tests say SKIP if OpenMPI is missing
make test M=14 T=ex3_ra_simulation
make run  M=14 EX=ex3_ra_simulation   # prints a small run message by message
make tsan M=14 T=ex4_token_ring       # the token ring must have no data races
```

Then do the [quiz](quiz.md) and check yourself with the
[recall sheet](recall.md). Answers are in
[`answers/14-distributed-mutual-exclusion/`](../../answers/14-distributed-mutual-exclusion/),
but try first.
