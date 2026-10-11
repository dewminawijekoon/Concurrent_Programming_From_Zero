# 09 · Classical synchronization problems ★

| | |
|---|---|
| **Time** | about 6 hours |
| **You should know first** | module 07 (semaphores and the patterns: signalling, mutex, multiplex, lightswitch, turnstile) |
| **Lectures** | L08 (semaphores) slides 26–35; L03 *Concurrent Programming Abstraction* slides 10–11 |
| **Helps with** | **Lab 04 Ex 1–4** (bridge, construction zone, barbershop, river crossing); exam questions like "design a solution and justify it", "can this deadlock?", "can a writer starve?" |

**Goal:** solve the famous synchronization puzzles with semaphores, and
explain *why* each solution is safe, cannot deadlock, and (where it
matters) cannot starve anyone.

Words in **bold** link to the [glossary](../../GLOSSARY.md) the first time
they appear. Module 07 explains the patterns used here; read its section
3.4 first if they are not fresh.

---

## 1. The problem

Real programs rarely need "just a lock". They need rules like "many
readers **or** one writer", "at most 5 on the rope, all going the same
way", "the boat leaves only with a valid group of 4". The classical
problems are small stories that capture such rules (L03 slide 10). They
are the exam's favourite design questions, because a solution that *looks*
right often has a hidden [**deadlock**](../../GLOSSARY.md#deadlock) or
lets one kind of thread [**starve**](../../GLOSSARY.md#starvation).

The method is always the same:
1. Write the rules down as a list. Mark each one as **safety** ("never
   …") or **liveness** ("eventually …"), as in module 05.
2. Build the solution from the module 07 patterns.
3. For each rule, say in one or two sentences why it holds.

## 2. The problems at a glance

| Problem | Safety rules | Liveness rules | Patterns used |
|---|---|---|---|
| Readers–writers (L08 slides 29–31) | a writer is alone; readers may share | readers get in; writers get in (not in the lecture's version!) | lightswitch; + turnstile for fairness |
| Dining philosophers (L08 slides 26–28) | a fork is used by one philosopher at a time | no deadlock; no starvation | one mutex per fork; a fixed order of picking up |
| Sleeping barber (L08 slides 32–34) | at most M waiting; one haircut at a time | the barber does not sleep while someone waits; every seated customer gets a haircut | signalling; rendezvous; a counter under a mutex |
| Baboon crossing (L08 slide 35) | one direction at a time; at most 5 on the rope | no direction waits forever | lightswitch per direction; multiplex; turnstile |
| River crossing (Lab 04 Ex 4) | every boat is 4H, 4S or 2H+2S | everyone crosses (if the numbers allow it) | counters under a mutex; queues; the captain keeps the mutex |

## 3. How to solve each one

### 3.1 Readers and writers (L08 slides 29–31)

Many threads share a database. **Readers** only look; **writers** change
it. Two readers can safely read at the same time. A writer must be alone:
no other writer, no reader.

**Too strict** (slide 30): one semaphore `rw = 1` around every read and
every write. It is safe, but readers can no longer read together. The
slide calls this *over-constrained*.

**Readers' preference** (slide 31): the lightswitch from module 07.

```c
int nr = 0;                       // readers inside
semaphore rw = 1, mutexr = 1;     // rw: the room; mutexr protects nr
reader:  down(mutexr); nr++; if (nr == 1) down(rw); up(mutexr);   // first in locks the room
         read;
         down(mutexr); nr--; if (nr == 0) up(rw); up(mutexr);     // last out unlocks it
writer:  down(rw);  write;  up(rw);
```

**Why it is safe.** A writer is inside only while it holds `rw`. Readers are
inside only while the *group* holds `rw` (from the first reader's
`down(rw)` to the last reader's `up(rw)`). `rw` starts at 1, so it is held
by one writer or by the reader group, never both.

**Why a writer can starve.** Look at this timeline:

```
time:   0    2    4    6    8    10   12
R1:     [=========]                         reads 0..5
W:           waits.............................   (arrived at 2)
R2:          [=========]                    reads 3..8
R3:                 [=========]             reads 6..11
R4:                       [=========]       ... and so on
```

Each new reader arrives while another reader is still inside, so `nr`
never drops to 0, `up(rw)` never happens, and W waits for ever. Every
reader finishes; the writer never starts. The slides do not mention this
(section 7).

**No-starve version: add a turnstile** (exercise 1). [EXTRA: the idea is
from *The Little Book of Semaphores*; the notes only have slide 31.]

```c
semaphore turnstile = 1;
reader:  down(turnstile); up(turnstile);          // pass through (blocked while a writer waits)
         ...lightswitch lock... read ...lightswitch unlock...
writer:  down(turnstile);                          // close the turnstile behind me
         down(rw);                                 // wait for the readers already inside
         write;
         up(turnstile); up(rw);
```

A waiting writer holds the turnstile, so readers who arrive **after** it
queue up at the turnstile. The readers already inside finish, the room
empties, and the writer gets in. Then the queued readers go in together.

| | slide 30 (one semaphore) | slide 31 (readers' preference) | with turnstile |
|---|---|---|---|
| readers read together | no | yes | yes |
| a writer can starve | no (if the semaphore is fair) | **yes** | no (if the semaphores are fair) |
| a reader can starve | no | no | no |

**[EXTRA] Writers' preference** (Lab 02 asks for it with condition
variables, module 08): as soon as one writer is waiting, no new reader may
start, even if other writers keep coming. With semaphores you need a
second lightswitch for writers that locks a "no new readers" semaphore.
Now *readers* can starve. Which one you want depends on the application;
the turnstile version is the fair middle ground.

### 3.2 Dining philosophers (L08 slides 26–28; L03 slide 11)

Five philosophers sit at a round table and alternate between thinking and
eating. There is one fork between each pair of neighbours, so five forks.
To eat, a philosopher needs **both** forks next to it. Fork i lies between
philosopher i and philosopher i+1.

Each fork is a binary semaphore that starts at 1. The obvious solution:

```c
philosopher i:  down(fork[i]);  down(fork[(i+1) % 5]);  eat;  up(both);  think;
```

**It can deadlock.** Every philosopher picks up its left fork at about the
same time:

| step | P0 | P1 | P2 | P3 | P4 |
|--:|---|---|---|---|---|
| 1 | takes fork 0 | takes fork 1 | takes fork 2 | takes fork 3 | takes fork 4 |
| 2 | waits for fork 1 (P1 has it) | waits for fork 2 | waits for fork 3 | waits for fork 4 | waits for fork 0 (P0 has it) |

Each philosopher holds one fork and waits for the next one, all the way
round the table. This *circle of waiting* can never untangle. Module 10
calls it a **circular wait**, one of the four conditions for deadlock.
Model exercise m1 shows the explorer finding this trace for 3
philosophers.

**The asymmetric solution** (slide 28): philosophers 0–3 pick up fork i
first, then fork i+1. Philosopher 4 picks up **fork 0 first**, then fork 4.
In other words, *everybody picks up the lower-numbered of its two forks
first*.

**Why there is no deadlock.** Suppose there were a circle of waiting. Each
philosopher in it holds a fork and waits for a **higher-numbered** fork
(it took the lower one first). Going round the circle, the fork numbers
would have to keep going up and still come back to the start. That is
impossible. Concretely: P0 and P4 both want fork 0 first. Whoever loses
that race holds **no** fork at all, so it cannot be part of a circle, and
its other neighbour can always get both forks.

**Why no two neighbours eat at once.** They share a fork, and a fork is a
binary semaphore: `#holding + fork = 1`, as in module 07.

**Starvation.** With first-come-first-served (blocked-queue) semaphores,
nobody starves. With busy-wait semaphores a philosopher can keep missing
its chance; model m2 shows the explorer finding such a loop.

**[EXTRA] The "waiter" solution.** Put a multiplex of 4 (module 07) around
picking up the forks: at most 4 of the 5 philosophers may *try* to eat at
once. With 4 philosophers and 5 forks, at least one of them can get both
forks (if each of the 4 held only one fork, one fork would still be on the
table next to someone who is trying). So a circle of all 5 can never form.

### 3.3 The sleeping barber (L08 slides 32–34; Lab 04 Ex 3)

A barber shop has one barber, one barber chair and some waiting chairs.
- A customer arrives. If all waiting chairs are taken, it leaves.
  Otherwise it sits down and waits.
- If nobody is waiting, the barber sleeps. A customer who sits down
  wakes him.
- The barber takes the next waiting customer and cuts its hair.

**The lecture's solution** (slide 33), in words:

| Variable | Start | Meaning |
|---|:--:|---|
| `waiting` | 0 | customers sitting in waiting chairs |
| `mutex` | 1 | protects `waiting` |
| `customers` | 0 | counts waiting customers; the barber sleeps on it |
| `barber` | 0 | the barber says "next, please"; a customer waits on it |

```
customer:  down(mutex)
           if waiting < chairs:  waiting++; up(customers); up(mutex); down(barber)   // sit, wake barber, wait to be called
           else:                 up(mutex)                                           // full: leave
barber:    loop: down(customers)            // sleep until someone is waiting
                 down(mutex); waiting--; up(barber); up(mutex)                      // call the next one
```

The customer and the barber meet in a **rendezvous**: the customer waits
for `barber`, the barber waits for `customers` (slide 34).

**Why at most `chairs` customers wait:** `waiting` only grows when it is
below `chairs`, and the test and the update happen under the same mutex.

**Why the barber does not sleep while someone waits:** every customer that
sits does `up(customers)`, so `customers` counts the waiting customers
that the barber has not yet called. The barber sleeps only when that
count is 0.

**What the slide leaves out** (and Lab 04 Ex 3 wants): the haircut itself,
and the customer waiting until the haircut is *finished*. Lab track 3 adds
two semaphores: `seated` (the customer says "I'm in the chair", and leaves
its id where the barber can read it) and `cut_done` (the barber says
"finished").

**A trap we fell into ourselves.** A token on a semaphore is not addressed
to anyone: whoever waits first takes it. In our first version, the barber
posted `cut_done` and called the next customer straight away. If the old
customer was slow to wake up, the **new** customer took the old one's
`cut_done` token and left before its own haircut had started. The lab
test caught it. Fix: the customer posts `left_chair` when it gets up, and
the barber waits for that before calling the next customer.

**Multiple barbers** (Lab Q3.3). With K barbers, "which customer is in
which chair" matters even more. The cleanest design gives each customer
its **own** semaphore:

```
customer id:  down(mutex)
              if waiting == chairs: up(mutex); leave
              waiting++; put id at the back of a queue; up(customers); up(mutex)
              down(done[id])                        // wait for MY haircut to finish
barber (K of them):
              loop: down(customers)
                    down(mutex); id = take the front of the queue; waiting--; up(mutex)
                    cut hair of id
                    up(done[id])                    // wakes exactly that customer
```

`customers` now wakes up to K barbers at once, the queue (under the
mutex) makes sure no two barbers take the same customer, and `done[id]`
can only wake the right customer. The queue also serves customers in
arrival order.

### 3.4 The baboon crossing (L08 slide 35; Lab 04 Ex 1–2)

Baboons cross a canyon on a rope. Rules: (1) never baboons going opposite
ways on the rope; (2) at most 5 on the rope; (3) a stream of baboons one
way must not keep the other way waiting for ever. The slide gives no
solution. Let's build one, one rule at a time.

- **Rule 1** is readers–writers with **two groups of readers**: baboons
  going the same way may share, the two directions exclude each other.
  So: one lightswitch per direction, both on the same semaphore
  `rope_free = 1`. The first baboon of a direction locks the rope for its
  whole direction; the last one off unlocks it.
- **Rule 2** is a multiplex: `rope = 5`. Take a place *after* your
  direction owns the rope.
- **Rule 3** is the turnstile, exactly as for readers–writers. A baboon
  that must wait for the other direction waits while holding the
  turnstile, so later baboons from its own side cannot sneak past.

```c
semaphore turnstile = 1, rope_free = 1, rope = 5;
lightswitch east, west;           // each = a counter + its own mutex
baboon going east:
    down(turnstile);
    lightswitch_lock(east, rope_free);   // may wait here, holding the turnstile
    up(turnstile);
    down(rope);   cross;   up(rope);
    lightswitch_unlock(east, rope_free);
```

Without the turnstile line, this is lab track 1 (Lab 04 Ex 1, capacity 3:
a direction can starve, Lab Q1.2). With it, it is lab track 2 (Lab 04 Ex
2, capacity N). `demo/baboons.il` checks every order of steps for a small
version (3 east, 2 west, rope of 2): the asserts for rules 1 and 2 always
hold and there is no deadlock. Change `rope = 2` to `rope = 3` and the
capacity assert fails.

### 3.5 River crossing (Lab 04 Ex 4)

Hackers and serfs share a boat that holds exactly 4. Valid groups: 4
hackers, 4 serfs, or 2 + 2. The design (lab track 4):

- Counters `hackers` and `serfs` (people waiting, not yet in a group),
  protected by `mutex`.
- Queue semaphores `hacker_q` and `serf_q`, both 0.
- When an arriving thread **completes** a valid group, it becomes the
  captain: it subtracts the group from the counters and posts the queue
  semaphores once per seat (its own seat too). It **keeps the mutex**.
- Everyone else releases the mutex and waits on its queue.
- Each chosen thread boards, then posts `boarded`; the captain waits for 3
  posts, rows, and only then releases the mutex.

**Why only valid groups board.** Only a captain posts the queues, and only
for a valid group. While the boat is filling, the captain holds the mutex,
so no new arrival can even update a counter, let alone form a second
group.

**Can everyone cross?** Every group takes an even number of hackers and an
even number of serfs. So if H or S is odd, someone is always left over.
Example: H = 5, S = 3 (8 people). After any first group (4H, or 2H+2S),
the 4 left are 1H+3S or 3H+1S: not valid. They wait for ever (Lab Q4.2:
"deadlock" only in the sense that they wait for people who never come).
If H and S are both even and H + S is a multiple of 4, everyone crosses
(e.g. H = 6, S = 2: 4H then 2H+2S, or 2H+2S then 4H).

## 4. Why it works: the justification checklist

For a design question, answer these one by one:

1. **Each safety rule.** Name the semaphore that enforces it and the
   invariant (e.g. "`rope_free` is held either by the east group or by the
   west group, never both, because it starts at 1").
2. **No deadlock.** Find every place a thread can wait. Show that someone
   always comes to wake it. Watch out for waiting on one semaphore while
   holding another (the dining circle; the mutex-before-`spaces` mistake
   in module 07).
3. **No starvation.** Ask: "can newcomers overtake a waiting thread for
   ever?" If yes, add a turnstile or a queue. State what you assume about
   the semaphore (first come, first served or not).
4. **Pairing.** If a token is meant for one particular thread, make sure no
   other thread can be waiting on the same semaphore at that moment, or
   give each thread its own semaphore (section 3.3).

## 5. API cheat-sheet

The same POSIX calls as module 07: `sem_init(&s, 0, value)`, `sem_wait`,
`sem_post`, `sem_destroy`. Not available on macOS: use the dev container.

## 6. Common exam mistakes

- Saying the slide 31 readers–writers solution is "correct" without adding
  that **writers can starve**.
- In dining philosophers, saying "deadlock happens because two
  philosophers take the same fork". It cannot: forks are binary
  semaphores. Deadlock comes from the **circle** of each holding one fork
  and waiting for the next.
- Thinking the asymmetric solution works because philosopher 4 is
  "special". What matters is that **all** philosophers pick up their forks
  in one global order (lowest number first). Philosopher 4 is simply the
  one for whom "lowest first" means "right fork first".
- In the barber, testing `waiting < chairs` outside the mutex: two
  customers can both see a free chair and both sit on it.
- Taking the capacity place (multiplex) **before** the direction
  (lightswitch) on the bridge. It does not deadlock, but vehicles that are
  still waiting for their direction then use up capacity places, so fewer
  than N vehicles may be on the bridge than allowed.
- River crossing: forgetting that the captain must hold the mutex until
  the boat has left. Otherwise a 5th thread can be released into the
  same boat, or a second group can form while the first is boarding.
- Answering "can it starve?" with "no, semaphores are fair". POSIX
  semaphores promise no order, and the lecture's blocked-set semaphore
  allows starvation for 3 or more threads (module 07, section 3.3).

## 7. Careful: what the notes say

- **L08 slide 31** (readers–writers) never mentions starvation. With this
  solution **writers can starve**. Lab 02 asks for a writer-preference
  version for exactly this reason. Also in [NOTES_ERRATA.md](../../NOTES_ERRATA.md).
- **L08 slide 28** (asymmetric philosophers) writes `fork[i+1]` for
  philosophers 0–3 and handles philosopher 4 separately, so no `% 5` is
  needed. The figure's numbers (philosophers and forks 0–4) came out of the
  export as a jumbled list of digits; use the text: fork i is between
  philosopher i and philosopher i+1.
- **L08 slide 33** (sleeping barber) declares a semaphore `cutting` that is
  never used. It is harmless; ignore it. The slide also has no haircut
  step, the customer does not wait for the haircut to end, and each
  customer loops (`while (true)`), arriving again immediately instead of
  "coming back later" as slide 32 says. Lab track 3 is the complete
  version.
- **L08 slide 35** (baboon crossing) states the problem but gives no
  solution. Section 3.4 develops one; `demo/baboons.il` checks it.
- **L03 slides 10–11** only name the classical problems (and describe the
  philosophers in three lines); the solutions are all in L08.
- **Lab 04 Ex 1** is titled "Traffic Intersection", but it describes a
  single-lane **bridge**. Lab track 1 follows the description.

## 8. [EXTRA] Going further

- **Monitors** (module 08) solve the same problems with a lock and
  condition variables. Compare Lab 04 Q2.3: semaphores are compact, but
  fairness is easier to *see* and change with explicit condition checks.
- **Deadlock theory** (module 10): the dining philosophers are the
  standard example of a circular wait; ordering the resources (the
  asymmetric solution) is the standard cure.
- **Further reading:** Allen B. Downey, *The Little Book of Semaphores*,
  chapters 4–5 (readers–writers, dining philosophers, barbershop, river
  crossing, and many more puzzles).

## 9. Now do

| # | Exercise | Type | What you practise |
|---|---|---|---|
| 1 | `exercises/concept/ex1_rw_no_starve.c` | core | readers–writers where writers cannot starve (turnstile) |
| 2 | `exercises/concept/ex2_dining.c` | core | fix the deadlocking philosophers (slide 28) |
| L1 | `exercises/lab/lab1_bridge.c` | lab track | **Lab 04 Ex 1**: one-direction bridge, at most 3 cars, self-validating program |
| L2 | `exercises/lab/lab2_construction_zone.c` | lab track | **Lab 04 Ex 2**: capacity N from the command line, fair; timing table for N = 1, 3, 5 |
| L3 | `exercises/lab/lab3_barbershop.c` | lab track | **Lab 04 Ex 3**: barber, M chairs, served + turned away = C; table for M = 1, 3, 10 |
| L4 | `exercises/lab/lab4_river_crossing.c` | lab track | **Lab 04 Ex 4**: valid boat groups of 4 |
| m1 | `exercises/models/m1_dining3_naive.il` | model | see the deadlock trace for 3 philosophers |
| m2 | `exercises/models/m2_dining3_asymmetric.il` | model | prove the asymmetric fix deadlock-free for every order of steps |
| demo | `demo/baboons.il` | demo | the baboon solution, checked by the explorer |

```bash
make test M=09                                        # test all exercises
make test M=09 T=lab3_barbershop                      # test one exercise
make run  M=09 EX=lab2_construction_zone ARGS="table" # the Lab Q2.2 table
make run  M=09 EX=lab3_barbershop ARGS="table"        # the Lab Q3.1 table
make run  M=09 EX=lab4_river_crossing ARGS="6 2"      # Lab Q4.3
python3 tools/interleave/interleave.py modules/09-classical-problems/demo/baboons.il
```

Then do the [quiz](quiz.md) and check yourself with the
[recall sheet](recall.md). Answers are in
[`answers/09-classical-problems/`](../../answers/09-classical-problems/),
but try first.
