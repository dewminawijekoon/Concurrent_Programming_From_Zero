# interleave: try every possible order of steps

When two threads run together, their steps can happen in many different
orders. Each order is called an **interleaving**. A bug may show up in only
one of them.

Running a program many times and hoping to hit that one bad order is
unreliable. This tool instead **tries every order**, one by one, so its
answers are certain. Use it for questions like:

- Can two threads be in the critical section at the same time?
- Can the program get stuck (deadlock)?
- Can the program print `B B A`? Can it print `B A B A …` forever?
- What happens on a single CPU core where some threads have higher priority?

You write a small program in a simple pseudocode (close to the lecture
slides), and the tool tells you what can happen. When something can go
wrong, it shows the **shortest order of steps** that causes it.

It needs only Python 3.

```bash
python3 tools/interleave/interleave.py tools/interleave/programs/peterson.il       # check a lock
python3 tools/interleave/interleave.py tools/interleave/programs/attempt3.il       # see a deadlock
python3 tools/interleave/interleave.py tools/interleave/programs/puzzle_ab.il --outputs           # every possible output
python3 tools/interleave/interleave.py tools/interleave/programs/puzzle_ab.il --why "B B A"       # can it print this?
python3 tools/interleave/interleave.py tools/interleave/programs/puzzle_ab.il --infinite "B A"    # can it print B A B A ... forever?
python3 tools/interleave/interleave.py tools/interleave/programs/peterson.il --step               # you choose which thread runs next
```

## What the results mean

| Result | Meaning | Example in the lectures |
|---|---|---|
| **Mutual exclusion VIOLATED** | there is an order of steps that puts two threads in `critical` at the same time | L05 Attempt 2 |
| **DEADLOCK** | every thread that hasn't finished is waiting, and none can ever continue | L05 Attempt 3, LockOne |
| **BLOCKED FOREVER** | a thread waits forever because another thread stopped in its non-critical section (so it never gives way) | L05 Attempt 1, LockTwo |
| **LIVELOCK** | threads keep running forever, but none ever gets into `critical` | L05 Attempt 4 |
| **STARVATION** | one thread keeps trying but never gets in, while other threads get in again and again | L06 (starvation-freedom) |

**About livelock and starvation.** These results assume a *fair* scheduler:
a thread that is ready to run all the time will get a turn eventually.
Without this assumption, every program could "starve" a thread simply
because the scheduler never picks it, which tells us nothing about the
algorithm. For these results the tool prints two parts: the steps that lead
into the loop, and the loop itself, which can repeat forever.

**How the tool counts steps.** It uses the same simple model as the
lectures:
- **Each line is one step that cannot be interrupted** (an atomic step):
  an assignment, a test, an `await`, a `print`, or a semaphore operation.
  So `x = x + 1` written on one line counts as one step. To show the race
  in `x++`, split it into separate read and write lines using a `local`
  variable (see `programs/counter_race.il`).
- A thread **may stop forever at `noncritical`**, as the lecture's rules
  allow. Add `--no-halt` to switch this off.

## Writing a program

```python
# Lines starting with # are comments.
const N = 3                      # a constant
shared x = 0, done = false       # shared variables: every thread sees the same ones
shared flag[N] = false           # an array of N elements, all false
sem mutex = 1, items = 0         # semaphores with their starting values
mutex m                          # short for: sem m = 1

proc lock_it(s):                 # a procedure; its body is copied in wherever it is called
    s.down()

thread A:                        # a thread called A
    print A

thread P(i) for i in 0..N-1:     # N threads P0, P1, P2; in each one, i is its number
    local tmp = 0                # a variable only this thread can see (must come first)
    loop:                        # repeat forever
        noncritical
        flag[i] = true
        await not flag[1-i]      # wait here until the condition is true
        critical
        flag[i] = false
```

| You write | It means |
|---|---|
| `x = e`, `a[i] = e`, `x++`, `x--`, `x += e` | set a variable (one step) |
| `await cond` | wait until `cond` is true. Use it for the busy-wait loops on the slides, like `while (flag[j]) {}`. |
| `while cond:`, `if cond:`, `elif cond:`, `else:` | as in Python; checking the condition is one step |
| `loop:` | repeat forever |
| `for v in LO..HI:` | count from LO to HI, including HI (like Filter's `for L = 1 .. n-1`) |
| `break` / `return` | leave the loop / leave the procedure |
| `atomic:` with an indented block | run the whole block as **one** step. Use it for hardware instructions like TestAndSet. |
| `critical`, `noncritical` | mark the critical section and the non-critical section |
| `print A`, `print "some text"`, `print x` | print a word, some text, or a variable's value |
| `s.down()`, `down(s)`, `P(s)`, `wait(s)` | semaphore down: wait while `s` is 0, then subtract 1 |
| `s.up()`, `up(s)`, `V(s)`, `signal(s)` | semaphore up: add 1 |
| `assert cond` | report a problem if `cond` can ever be false at this point |
| `skip`, `sleep N` | do nothing (time is not modelled) |

Conditions can be written like Python (`and`, `or`, `not`) or like C (`&&`,
`||`, `!`). Comparisons work as usual. A pair like `(label[k], k) < (label[i], i)`
compares the first values and uses the second ones only to break a tie,
just as in the Bakery lock. `max(label)` and
`any(level[k] >= L for k in range(N) if k != i)` also work. A `;` at the end
of a line is allowed, so code copied from C needs little editing.

## Options

| Option | What it does |
|---|---|
| `--outputs` | list every output the program can print when all threads finish, and any loops that run forever |
| `--why "B B A"` | can the program print exactly this? If yes, show the order of steps |
| `--infinite "B A"` | can the program print `B A B A …` forever? |
| `--step` | you choose which thread runs next (`u` = undo, `q` = quit) |
| `--priority A=2,C=1` | one CPU core with priorities: only the highest-priority threads that are ready may run |
| `--no-halt` | threads may not stop at `noncritical` |
| `--bound N` | ignore states where some number grows bigger than N (Bakery's ticket numbers grow forever) |
| `--max-states`, `--max-output`, `--show` | size limits |

## Limits

- The tool checks **the program you wrote for it**, not your C code. Copy
  the C code line by line, with one shared read or write per line, and the
  answer applies to the real algorithm.
- It uses the lecture's simple memory model. Real CPUs can reorder reads and
  writes unless you use atomic variables; see module 05, section 6.
- Time is not modelled. For questions like "T1 starts at 0 s, T2 at 10 s",
  work it out by hand first, then use `--why` to check that order is
  possible.
- The number of possible orders grows very quickly. Keep programs small:
  2–4 threads and small numbers.

## Example programs

`programs/` has the lock attempts from the lectures (Attempts 1–4, Dekker,
Peterson, LockOne, LockTwo, Filter, Bakery), TestAndSet locks, the counter
race, an output puzzle, and the lightswitch pattern. `tests/run.sh` checks
that the tool's answers match the lecture notes (`make interleave-selftest`).
