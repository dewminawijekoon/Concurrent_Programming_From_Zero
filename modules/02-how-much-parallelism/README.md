# 02 · How much parallelism? ★

| | |
|---|---|
| **Time** | about 4 hours |
| **You should know first** | module 00 (cores, threads); module 01 helps (why locks make code serial) |
| **Lectures** | L02 *How Much Parallelism?* (slides 1–41) |
| **Helps with** | Lab 01 Ex 4 (Amdahl's law analysis); exam questions like "8 processors at 60% parallel, or 4 processors at 80%: which is better?", "work, span and parallelism of this DAG", "how many runs do you need?"; module 11 |

**Goal:** predict how much faster a program can get on more processors,
explain what stops it from getting faster, and measure performance in a way
you can trust.

Words in **bold** link to the [glossary](../../GLOSSARY.md) the first time
they appear.

---

## 1. The problem

You have 10 processors. You work hard and make 60% of your program's
running time run in parallel. How much faster is it now?

Not 10 times. Not 6 times. It is **2.17 times** faster (L02 slide 6). And
with a million processors it would still be only **2.5 times** faster.

Why so little? The other 40% still runs on one processor, and it takes the
same time as before. Make that 40% concrete: say the program took 100 s.
The serial 40 s stay 40 s. The parallel 60 s shrink to 6 s on 10
processors. Total: 46 s. 100 / 46 = 2.17.

This module gives you the tools to see this coming: **speedup**,
**efficiency**, **Amdahl's law**, **work and span**, and how to measure
running times properly.

## 2. The idea: the measures

| Measure | Formula | Plain meaning |
|---|---|---|
| **Speedup** | S = T₁ / Tₙ | how many times faster on n processors than on 1. T₁ = time on 1 processor, Tₙ = time on n. |
| **Efficiency** | E = S / n | how much of the processors' time is useful work. 1 means perfect; 0.25 means three quarters of the time is wasted. |
| **Parallel fraction** p | | the share of the *running time* on one processor that can run in parallel. 0.9 means 90%. (It is about time, not lines of code: L02 slide 6, speaker notes.) |
| **Amdahl's law** | S = 1 / ((1 − p) + p / n) | the best speedup with n processors when a fraction 1 − p must run serially |
| **Amdahl's limit** | S ≤ 1 / (1 − p) | the best speedup with unlimited processors |
| **Work** | T₁ | total time on one processor (L02 slide 20) |
| **Span** | T∞ | time with unlimited processors: the longest chain of steps that must happen one after another (slide 20) |
| **Parallelism** | T₁ / T∞ | the best speedup *any* number of processors can give (slide 26) |
| **Latency** | | time to finish **one** job (slide 31) |
| **Throughput** | | number of jobs finished **per unit of time** (slide 31) |

## 3. The mechanism

### 3.1 Speedup and efficiency (L02 slides 23–25)

A program takes 120 s on 1 processor and 40 s on 4 processors.

- Speedup = 120 / 40 = **3**.
- Efficiency = 3 / 4 = **0.75**. A quarter of the processors' time is lost
  (waiting, communicating, doing extra work).

Speedup is normally **at most n** (slide 23): n processors can do at most n
times as much work per second. If S = n exactly, that is *perfect linear
speedup*. Efficiency is between 0 and 1 (slide 25).

### 3.2 Amdahl's law (L02 slides 3–11)

**Where the formula comes from.** Say the program takes time 1 on one
processor. A fraction p of that time can be split between processors; the
rest, 1 − p, cannot.

```
one processor:   |<------ 1 - p (serial) ------>|<----------- p (parallel) ----------->|
n processors:    |<------ 1 - p (serial) ------>|<- p/n ->|
```

On n processors the serial part still takes 1 − p. The parallel part takes
p / n. So

```
Tn = (1 - p) + p / n          speedup S = 1 / Tn = 1 / ((1 - p) + p / n)
```

**The lecture's examples** (10 processors, slides 6–9):

| parallel fraction p | serial 1 − p | speedup | efficiency | limit 1 / (1 − p) |
|--:|--:|--:|--:|--:|
| 60% | 40% | 2.17 | 0.22 | 2.5 |
| 80% | 20% | 3.57 | 0.36 | 5 |
| 90% | 10% | 5.26 | 0.53 | 10 |
| 99% | 1% | 9.17 | 0.92 | 100 |

Worked example for 90%: (1 − 0.9) + 0.9 / 10 = 0.1 + 0.09 = 0.19, and
1 / 0.19 = 5.26.

**The limit** (slide 10). As n grows, p / n shrinks towards 0, and only the
serial part is left: S approaches 1 / (1 − p). With just 5% serial, no
number of processors can give more than 20× speedup.

**The moral** (slide 11). Going from 90% to 99% parallel nearly doubles
the speedup on 10 processors (5.26 → 9.17). The last few percent of serial
code matter a lot. They are often the hard parts: code that coordinates
threads on shared data, such as code inside a lock (module 01). Reducing
that serial part is worth real effort.

**Exam-style comparison (our own numbers).** Option A: 16 processors, 70%
parallel. Option B: 6 processors, 90% parallel. Which is faster?

```
A: 1 / (0.3 + 0.7/16) = 1 / (0.3 + 0.04375) = 1 / 0.34375 = 2.91
B: 1 / (0.1 + 0.9/6)  = 1 / (0.1 + 0.15)    = 1 / 0.25    = 4.00
```

B wins, with fewer than half the processors. Its limit is also much higher
(10 against 3.33), and its efficiency is 0.67 against 0.18. Making more of
the program parallel beat buying more processors.

```bash
make run M=02 EX=amdahl ARGS="compare 0.7 16 0.9 6"   # prints this worked answer
make run M=02 EX=amdahl ARGS="need 0.9 5"             # how many processors for 5x?
```

### 3.3 What kills parallelism (L02 slides 11–13, 30)

| Cause | What happens | Example |
|---|---|---|
| **Serial parts** | some work cannot be split | reading the input file; Amdahl's 1 − p |
| **Synchronization and contention** | threads wait for a lock or for each other | the global mutex in Lab 01 Ex 3 (module 01) |
| **Communication** | moving data between processors takes time | sending messages between machines (module 13) |
| **Start-up and termination** | creating and joining threads costs time | starting a thread to add two numbers (exercise 4) |
| **Load imbalance** | some threads get more work, others wait for them | the static prime split (module 00, exercise 2) |
| **Bottlenecks** | a shared resource is full | memory bandwidth, network bandwidth, memory size (slide 30) |

Because of these costs, efficiency is usually below 1 (slide 12). Parallel
programs are also harder to design, debug and maintain (slide 13). So a
10–20% gain is usually not worth it.

### 3.4 Work and span: the P-FIB example (L02 slides 14–26)

**The program** (slides 14, 16). Fibonacci numbers: F₀ = 0, F₁ = 1,
Fₙ = Fₙ₋₁ + Fₙ₋₂. The parallel version:

```
P-FIB(n):
    if n <= 1: return n
    x = spawn P-FIB(n - 1)   // "spawn": this call MAY run in parallel with what follows
    y = P-FIB(n - 2)         // a normal call, in the current thread
    sync                     // wait until the spawned call has finished
    return x + y
```

**The computation DAG** (slides 18–19). DAG means *directed acyclic graph*:
dots joined by one-way arrows, with no way to go round in a circle.

- Each dot (vertex) is a **strand**: a piece of code with no spawn and no
  sync inside it. Here each strand takes 1 time unit.
- An arrow u → v means "u must finish before v can start".
- A call with n ≤ 1 is **one** strand. A call with n ≥ 2 is **three**:
  **A** (test n, spawn), **B** (call P-FIB(n − 2)), **C** (after the sync,
  return x + y).

The calls made by P-FIB(4), with their strands:

```
P-FIB(4)  A B C
├── spawn P-FIB(3)  A B C
│   ├── spawn P-FIB(2)  A B C
│   │   ├── spawn P-FIB(1)  (1 strand)
│   │   └── call  P-FIB(0)  (1 strand)
│   └── call  P-FIB(1)  (1 strand)
└── call  P-FIB(2)  A B C
    ├── spawn P-FIB(1)  (1 strand)
    └── call  P-FIB(0)  (1 strand)
```

**Work** = number of strands (slide 20). There are 4 calls with n ≥ 2
(3 strands each) and 5 calls with n ≤ 1 (1 strand each):
4 × 3 + 5 × 1 = **17** (slide 21).

**Span** = the number of strands on the longest path (slide 20). Inside
each call with n ≥ 2 there are two routes from A to C:

```
A -> (the spawned child, all of it) -> C           = 2 + span(n - 1)
A -> B -> (the called child, all of it) -> C       = 3 + span(n - 2)
```

The span is the longer route. Work it out from the bottom:

| n | span(n) | longer route |
|--:|--:|---|
| 0, 1 | 1 | a single strand |
| 2 | 4 | max(2 + 1, 3 + 1) = 4: A → B → P-FIB(0) → C |
| 3 | 6 | max(2 + 4, 3 + 1) = 6 |
| 4 | 8 | max(2 + 6, 3 + 4) = 8 |

So span = **8** (slide 21). One longest path is: A of P-FIB(4) → A of
P-FIB(3) → A of P-FIB(2) → B of P-FIB(2) → P-FIB(0) → C of P-FIB(2) → C of
P-FIB(3) → C of P-FIB(4).

**Parallelism** = work / span = 17 / 8 = **2.125** (slide 26). However many
processors you use, P-FIB(4) cannot run more than about 2 times faster.
For bigger n the work grows very fast (P-FIB(30) has over 5 million
strands) but the span only grows by 2 per step (span(30) = 60), so the
parallelism becomes huge. Exercise 4 computes this.

**Two laws** that bound the time Tₚ on p processors (slides 22, 25):

| Law | Statement | Why |
|---|---|---|
| **Work law** | Tₚ ≥ T₁ / p | in one time step, p processors can do at most p strands. All T₁ strands must be done. |
| **Span law** | Tₚ ≥ T∞ | the strands on the longest path must run one after another, however many processors there are. |

So Tₚ ≥ max(T₁ / p, T∞). For P-FIB(4) on 2 processors: max(8.5, 8) = 8.5.
On 4 processors: max(4.25, 8) = 8. More processors stop helping once
p > parallelism (slide 26).

**Which strands are "in parallel"?** (slide 18). If there is a path of
arrows from u to v, then u and v are *in series*: u must come first.
Otherwise they are *logically in parallel*: they may run at the same time.

### 3.5 Latency, throughput and scalability (L02 slides 27–32)

- **Latency** is the time for one job. Measure it when a user is waiting,
  when there is a deadline, or when there is only one job (slide 32).
- **Throughput** is jobs per second. Measure it when many jobs run and
  overall use of the machine matters, such as a web server (slide 32).
- If one job can be split into tasks, more resources reduce its latency and
  raise throughput. If the parallelism is only "more independent jobs at
  once" (data level, many requests), more resources raise throughput but
  each job takes as long as before (slide 27).
- **Scalability** (slides 28–30): when the problem gets bigger, can you keep
  up by adding resources? You show it with a graph of cost against problem
  size. Scalability is limited by **bottlenecks**: CPU, the memory bus,
  network bandwidth, memory size. Remove one, and the next one appears.

### 3.6 Measuring performance properly (L02 slides 33–41)

A running time is a random measurement: background programs, caches and
the scheduler change it every run. So one run tells you little.

**Repeat and summarise.** Run the program several times. Report:

| Number | What it tells you |
|---|---|
| **mean** (average) | the typical time |
| **median** (middle value when sorted) | the typical time, ignoring a few very slow runs |
| **standard deviation** s | how spread out the times are |
| min, max, percentiles | the extremes and the shape |

The sample standard deviation is s = √( Σ (xᵢ − mean)² / (n − 1) ). We divide
by n − 1, not n, because the mean was estimated from the same data, which
makes the spread look slightly smaller than it really is.

**Confidence interval** (slides 33–34). The *Central Limit Theorem* says:
if you average n measurements, the average behaves like a value from a
bell-shaped (normal) curve around the true mean, and its spread shrinks as
n grows (its standard deviation is s / √n). So we can say

```
true mean is in   mean ± z · s / √n    with confidence 95% if z = 1.96
                                      (90%: z = 1.645, 99%: z = 2.576)
```

Example: times 18, 22, 20, 25, 15 s. Mean 20, s = 3.81. The 95% interval is
20 ± 1.96 × 3.81 / √5 = 20 ± 3.34, so **16.7 s to 23.3 s**. Five runs leave a
lot of uncertainty.

**How many runs?** (slides 35–36). To get the mean within ± r percent with
confidence z:

```
n = ( 100 · z · s / (r · mean) )²       then round UP
```

Slide 36 example: mean 20 s, s = 5 s, accuracy ± 1 s, which is 5% of 20, at
95% (z = 1.96):

```
n = (100 × 1.96 × 5 / (5 × 20))² = (980 / 100)² = 9.8² = 96.04  ->  97 runs
```

Always round **up**: 96 runs would give slightly less than the accuracy you
asked for.

**Presenting results** (slides 37–41).
- Tables for exact comparisons; graphs to show trends.
- **Error bars** show the uncertainty of each point (for example the
  confidence interval). **Box plots** show the median, the middle half of
  the values (the box), the range (the whiskers) and outliers.
- Graph rules: a title or caption; both axes labelled **with units**; the
  thing you control (for example the number of threads) on the x-axis; time
  as a variable always on the x-axis; sensible tick marks; a key for colours
  and symbols; the axes need not start at zero; error bars encouraged.

## 4. Why it works

**Amdahl's law** follows from one assumption: the serial part takes the
same time on any number of processors, and the parallel part divides
perfectly. Both are *optimistic* (no overheads), so the real speedup is
usually lower. That is why it is called the *maximum* speedup.

**Work law.** Each processor does at most one unit of work per time unit,
so p processors do at most p · Tₚ units in time Tₚ. All T₁ units must be
done, so p · Tₚ ≥ T₁, which gives Tₚ ≥ T₁ / p (slide 25).

**Span law.** The strands on the longest path depend on each other in a
chain: each can only start after the previous one ends. Even with a
processor for every strand, the chain takes T∞ time units. So Tₚ ≥ T∞.

**Parallelism bounds the speedup.** Speedup = T₁ / Tₚ ≤ T₁ / T∞ (by the
span law) = parallelism.

## 5. Formula cheat-sheet

| What | Formula |
|---|---|
| speedup | S = T₁ / Tₙ |
| efficiency | E = S / n |
| Amdahl | S = 1 / ((1 − p) + p / n) |
| Amdahl's limit | 1 / (1 − p) |
| p from a measured speedup | p = (1 − 1/S) / (1 − 1/n) |
| p from timed phases | p = T_parallel / (T_serial + T_parallel), both on 1 processor |
| work law, span law | Tₚ ≥ T₁ / p, Tₚ ≥ T∞ |
| parallelism | T₁ / T∞ |
| confidence interval | mean ± z · s / √n |
| sample size | n = (100 · z · s / (r · mean))², rounded up |

## 6. Common exam mistakes

- Using p as the *serial* fraction. In Amdahl's formula p is the
  **parallel** fraction; the serial fraction is 1 − p.
- Saying "60% parallel on 10 processors gives 6×". Compute it: 2.17.
- Forgetting the limit: even unlimited processors cannot beat 1 / (1 − p).
- In a comparison question, computing only the speedups. Also give the
  recommendation, the limits, and the efficiency (what each option costs).
- Counting span as "the depth of the call tree". It is the number of
  **strands** on the longest path, and B strands count too (span of P-FIB(4)
  is 8, not 4).
- Rounding the sample size **down**. 96.04 means 97 runs.
- Dividing by n instead of n − 1 for the sample standard deviation.
- Putting the number of threads on the y-axis, or leaving out units.

## 7. Careful: what the notes say

- **Slides 4–9:** the Amdahl formula itself is an image and is missing from
  the export. Only the answers (2.17, 3.57, 5.26, 9.17) survive. The
  formula above is the standard one from the speaker notes, and it gives
  exactly those answers.
- **Slides 35–36:** the sample-size formula and the final answer are images.
  We reconstructed n = (100 z s / (r · mean))² from the slide's notes
  ("S – std, r – accuracy") and the example's numbers. It gives 96.04,
  rounded up to **97** runs.
- **Slide 26** says P-FIB(4)'s speedup "will not be more than 2". The exact
  bound is 2.125. In practice you cannot use a fraction of a processor, so
  "about 2" is the meaning.
- **Slide 22** says T₁ = work and also "time on best serial algorithm". These
  are not always the same: a parallel algorithm may do extra work that the
  best serial algorithm avoids. In this course, use T₁ = work of the DAG.
- **Slide 23** says speedup is at most P. That holds in the lecture's ideal
  model. On real machines you can sometimes see more (see section 8).
- **Slides 6–9** say "60% concurrent". This means 60% of the **running
  time**, not 60% of the code (speaker notes of slide 6).

## 8. [EXTRA] Beyond the slides

- **Gustafson's law.** Amdahl assumes a *fixed* problem size. Often we use
  more processors to solve a *bigger* problem in the same time (a finer
  weather grid). If p is the parallel fraction of the time measured on the
  n-processor run, the *scaled speedup* is (1 − p) + p · n. For p = 0.9 and
  n = 10 that is 9.1, against Amdahl's 5.26. Both are right; they answer
  different questions. `make run M=02 EX=amdahl ARGS="gustafson 0.9 10"`.
- **Estimating p from measurements** (Lab 01 Q4.2). Solving Amdahl's law for
  p gives p = (1 − 1/S) / (1 − 1/n). This is the idea behind the
  *Karp–Flatt metric*, which estimates the serial fraction from measured
  speedups. If the estimate of 1 − p *grows* with n, overheads (not just
  serial code) are hurting you.
- **Brent's bound.** A good scheduler achieves Tₚ ≤ T₁ / p + T∞. Together
  with the work and span laws this says Tₚ is within a factor of 2 of the
  best possible.
- **Superlinear speedup.** Sometimes S > n: with more processors, each one's
  share of the data may fit in its fast cache. This breaks the "ideal
  computer" assumption of slide 19, not the logic of the laws.
- **Small samples.** With fewer than about 30 runs, statisticians replace z
  by a slightly larger number from the *t-distribution* (for 5 runs at 95%,
  2.78 instead of 1.96). The lecture uses z.

## 9. Now do

| # | Exercise | Type | What you practise |
|---|---|---|---|
| 1 | `exercises/concept/ex1_amdahl.c` | warm-up | speedup, efficiency, Amdahl, the limit, processors needed |
| 2 | `exercises/concept/ex2_measuring.c` | core | mean, standard deviation, confidence interval, sample size |
| 3 | `exercises/concept/ex3_workspan.c` | core | work, span and parallelism of any DAG; the work and span laws |
| 4 | `exercises/concept/ex4_pfib.c` | challenge | P-FIB's work and span by formula, then P-FIB with real threads and a cutoff |
| L1 | `exercises/lab/lab1_amdahl_measure.c` | lab | Lab 01 Ex 4: time a serial and a parallel phase, estimate p, predict 64 processors |

```bash
make test M=02                                        # test all exercises
make test M=02 T=ex3_workspan                         # test one
make run  M=02 EX=amdahl ARGS="compare 0.6 8 0.8 4"   # calculator with worked answers
make run  M=02 EX=amdahl ARGS="samples 20 5 5"        # sample size (slide 36)
make run  M=02 EX=lab1_amdahl_measure                 # tables for Lab 01 Q4.1-Q4.3
make run  M=02 EX=ex4_pfib ARGS="25 2"                # 75,000 threads: watch the overhead
```

Then do the [quiz](quiz.md) and check yourself with the
[recall sheet](recall.md). Answers are in
[`answers/02-how-much-parallelism/`](../../answers/02-how-much-parallelism/),
but try first.
