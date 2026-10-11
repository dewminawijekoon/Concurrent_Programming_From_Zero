# interleave: try every schedule

A stress test runs your program thousands of times and *hopes* to hit the bad
interleaving. `interleave` tries **all** of them, so its answers are exact. It
is the tool for the questions that keep appearing in this course and its exams:

- Does this lock guarantee mutual exclusion? Can it deadlock? Livelock? Starve?
- Is output `B B A` possible? Can the program print `B A B A …` forever?
- What goes wrong on a single core with a priority scheduler?

```bash
python3 tools/interleave/interleave.py tools/interleave/programs/peterson.il
python3 tools/interleave/interleave.py tools/interleave/programs/attempt3.il      # deadlock trace
python3 tools/interleave/interleave.py tools/interleave/programs/puzzle_ab.il --outputs
python3 tools/interleave/interleave.py tools/interleave/programs/puzzle_ab.il --why "B B A"
python3 tools/interleave/interleave.py tools/interleave/programs/puzzle_ab.il --infinite "B A"
python3 tools/interleave/interleave.py tools/interleave/programs/peterson.il --step   # you are the scheduler
# or: make interleave F=tools/interleave/programs/peterson.il ARGS="--step"
```

It needs only Python 3 and no packages.

## What the verdicts mean

| Verdict | Meaning | Lecture link |
|---|---|---|
| **Mutual exclusion VIOLATED** | some schedule puts two threads in `critical` at once | safety (L01, L03) |
| **DEADLOCK** | every unfinished thread is waiting and nobody can move | L05 Attempt 3, LockOne |
| **BLOCKED FOREVER** | a thread waits forever because another stopped in its non-critical section | "success in the absence of contention" (L05): Attempt 1, LockTwo |
| **LIVELOCK** | threads keep running, nobody ever enters `critical` | L05 Attempt 4 |
| **STARVATION** | one thread is overtaken forever while others keep entering | starvation-freedom (L06) |

Livelock and starvation are checked under a **fair scheduler**: a thread that
can run all the time eventually gets to run. Without that assumption anything
could "starve" just because the scheduler ignores it. Each failure comes with
the **shortest schedule** that shows it. For livelock and starvation you get
a schedule that reaches a loop, plus the loop that repeats forever.

**Important modelling choices** (the same as the lectures):
- Every statement is **one atomic step**: an assignment, a test, `await`,
  `print`, or a semaphore operation. To show that `x = x + 1` is not atomic,
  split it into a load and a store through a `local` (see `counter_race.il`).
- `noncritical` is a step where a thread **may stop forever**, as the
  lecture's rules allow. Pass `--no-halt` to forbid that.

## The language

```python
# comments start with #
const N = 3                      # constants
shared x = 0, done = false       # shared variables (true/false, integers)
shared flag[N] = false           # arrays
sem mutex = 1, items = 0         # counting semaphores
mutex m                          # same as: sem m = 1

proc lock_it(s):                 # procedures are copied in at each call
    s.down()

thread A:                        # one thread
    print A

thread P(i) for i in 0..N-1:     # N copies: P0, P1, P2 (i is a constant in each)
    local tmp = 0                # thread-local variables (first lines only)
    loop:                        # repeat forever
        noncritical
        flag[i] = true
        await not flag[1-i]      # wait until the condition holds (blocking)
        critical
        flag[i] = false
```

| Statement | Meaning |
|---|---|
| `x = e`, `a[i] = e`, `x++`, `x--`, `x += e` | atomic assignment |
| `await cond` | wait until `cond` is true (busy-wait loops in the slides) |
| `while cond:` / `if cond:` / `elif` / `else:` | evaluating the condition is one step |
| `loop:` | infinite loop |
| `for v in LO..HI:` | counted loop, inclusive (as in Filter's `for L = 1..n-1`) |
| `break`, `return` | leave a loop / a procedure |
| `atomic:` + block | run the block as ONE step (models TestAndSet, CAS, atomic `++`) |
| `critical`, `noncritical` | critical-section and non-critical-section markers |
| `print A`, `print "text"`, `print x` | output a word, a string, or a variable's value |
| `s.down()` / `down(s)` / `P(s)` / `wait(s)` | semaphore down (blocks while 0) |
| `s.up()` / `up(s)` / `V(s)` / `signal(s)` | semaphore up |
| `assert cond` | report a failure if `cond` can ever be false here |
| `skip`, `sleep N` | do nothing (time is not modelled) |

Conditions use Python or C syntax: `and or not` or `&& || !`, `==`, `!=`,
`<`, and tuples for lexicographic order, e.g. `(label[k], k) < (label[i], i)`.
`max(label)` and `any(level[k] >= L for k in range(N) if k != i)` work too.
Lines may end with `;`, so code copied from C needs little editing.

## Options

| Option | Use |
|---|---|
| `--outputs` | list every output of runs where all threads finish, plus loops that run forever |
| `--why "B B A"` | is exactly this output possible? If yes, show a schedule |
| `--infinite "B A"` | can the output be `B A B A …` forever (fair schedule)? |
| `--step` | interactive: you pick which thread moves next (`u` undo, `q` quit) |
| `--priority A=2,C=1` | strict-priority single-core scheduler: only the highest-priority runnable threads run |
| `--no-halt` | threads may not stop in `noncritical` |
| `--bound N` | cut off states where a number exceeds N (Bakery labels grow forever) |
| `--max-states`, `--max-output`, `--show` | limits |

## Limits and honest caveats

- It checks **your model**, not your C code. Write the model line by line
  from the code, one shared access per statement, and it tells you the truth
  about that model.
- The model is **sequentially consistent** (the lecture model). Real CPUs
  reorder memory operations unless you use atomics; see
  `modules/05-mutual-exclusion/break_it/`.
- Time (`sleep`) is not modelled. For "T1 starts at 0 s, T2 at 10 s…"
  questions, reason by hand, then use `--why` to confirm the order is possible.
- State spaces explode quickly. Keep it to 2–4 threads and small numbers.

## Example programs

`programs/` holds the lecture protocols (Attempts 1–4, Dekker, Peterson,
LockOne, LockTwo, Filter, Bakery), TAS locks, a counter race, an output
puzzle and the lightswitch pattern. `tests/run.sh` checks that every verdict
matches the lecture notes (`make interleave-selftest`).
