# 02 · Quiz answers: how much parallelism?

These answers are about the length a marker expects for the marks shown.
"Why" is always part of the answer. Every number was checked with
`calc/amdahl.c` or the exercise solutions.

## A. True / false

1. **False.** 1 / (0.05 + 0.95 / 100) = 1 / 0.0595 = **16.8**. Even unlimited
   processors give at most 1 / 0.05 = 20.
2. **False.** Speedup is at most P in the ideal model (L02 slide 23), so
   efficiency = speedup / P is at most 1 (slide 25).
3. **False.** p is the fraction of the **running time** (on one processor),
   not of the code (L02 slide 6, speaker notes). A short loop can be 90% of
   the time.
4. **False.** Real overheads grow with more processors: starting threads,
   synchronization, communication, shared memory bandwidth, load
   imbalance (slides 12, 30). Efficiency drops below 1.
5. **True.** By the span law Tₚ ≥ T∞, so speedup T₁ / Tₚ ≤ T₁ / T∞ (slide 26).
6. **False.** If the parallelism is only "more independent jobs at once",
   more resources raise throughput but each job takes just as long, so
   latency does not improve (slide 27).
7. **False.** The half-width is z · s / √n. To halve it, √n must double, so
   you need **four times** as many runs.
8. **True.** The median is the middle value, so a few huge values move it
   little; one huge value pulls the mean up a lot.

## B. Calculations

9. Speedup = 1 / (0.2 + 0.8 / 10) = 1 / 0.28 = **3.57**. Efficiency =
   3.57 / 10 = **0.36**. Limit = 1 / 0.2 = **5**.
10. (a) A: 1 / (0.25 + 0.75 / 12) = 1 / 0.3125 = **3.20**.
    B: 1 / (0.1 + 0.9 / 4) = 1 / 0.325 = **3.08**.
    (b) Limits: A 1 / 0.25 = **4**; B 1 / 0.1 = **10**. B on 8 processors:
    1 / (0.1 + 0.1125) = **4.71**, already above A's limit.
    (c) **Recommend B**, even though A is slightly faster today (3.20 vs
    3.08). A is already close to its ceiling of 4: more hardware later
    gives almost nothing. B raises the ceiling to 10, so every future
    processor helps (4.71 on 8). B also uses its processors better
    (efficiency 0.77 against 0.27 for A). If the speedup were needed right
    now and nothing would change later, A would be the choice; say so.
11. Need 1 / (0.1 + 0.9 / n) ≥ 5, so 0.1 + 0.9 / n ≤ 0.2, so n ≥ 0.9 / 0.1 =
    **9** (8 gives 4.71). A speedup of 10 is **impossible**: the limit is
    1 / 0.1 = 10, reached only with infinitely many processors.
12. (a) p = 9 / (3 + 9) = **0.75**. (b) Speedup = 12 / 5.6 = 2.14;
    p = (1 − 1/2.14) / (1 − 1/4) = 0.533 / 0.75 = **0.71**.
    (c) 64: 1 / (0.25 + 0.75/64) = **3.82**; unlimited: 1 / 0.25 = **4**.
    (d) (a) assumes the parallel phase divides perfectly. The real 4-thread
    run also pays for starting and joining threads, waiting for the slowest
    chunk, and sharing memory bandwidth. Those overheads look like extra
    serial time, so (b) gives a smaller p. Timing noise also matters:
    repeat the runs (section 3.6).
13. Work = 1 + 2 + 3 + 1 + 4 + 2 = **13**. Paths: A-B-D-F = 6, A-C-D-F = 7,
    A-C-E-F = 1 + 3 + 4 + 2 = **10**. Span = **10** (critical path A → C → E →
    F). Parallelism = 13 / 10 = **1.3**. On 2 processors:
    T₂ ≥ max(13 / 2, 10) = **10** s. The span law decides; a second processor
    can save at most 3 s.
14. P-FIB(3) has strands A, B, C for P-FIB(3); A, B, C for its spawned
    P-FIB(2); and single strands for P-FIB(1), P-FIB(0) (children of
    P-FIB(2)) and P-FIB(1) (called by P-FIB(3)). Work = 2 × 3 + 3 = **9**.
    Span = max(2 + span(2), 3 + span(1)) = max(2 + 4, 3 + 1) = **6**.
    P-FIB(5): work = 3 + 17 + 9 = **29**; span = max(2 + 8, 3 + 6) = **10**;
    parallelism = **2.9**.
15. 95%: n = (100 × 1.96 × 4 / (2 × 50))² = 7.84² = 61.47 → **62** runs.
    99%: (100 × 2.576 × 4 / 100)² = 10.304² = 106.17 → **107** runs. Round up
    both times.
16. Mean = 48 / 4 = **12 s**. Squared differences: 0, 4, 1, 9 (sum 14);
    s = √(14 / 3) = **2.16 s**. Half-width = 1.96 × 2.16 / √4 = 2.12, so the
    interval is **9.88 s to 14.12 s**.

## C. Explain and design

17. Any four (L02 slides 11–13, 30): **serial parts** (reading the input);
    **synchronization / contention** (threads queueing for one lock, Lab 01
    Ex 3 Part A); **communication** (sending data between machines);
    **start-up and termination** (creating a thread per tiny task, P-FIB
    with cutoff 2); **load imbalance** (static prime split); **bottlenecks**
    (memory bus bandwidth).
18. (a) **Latency**: a user waits for each frame and there is a deadline.
    (b) **Throughput**: nobody waits for one photo; what matters is the total
    done by morning. (c) Mostly **throughput** (requests per second), but each
    user waits for their own page, so the latency of requests matters too:
    measure both (slide 32).
19. (a) p = 0.006 / (1.06 + 0.006) = **0.006** (0.6%). (b) Almost all the time
    is the serial sort. Making the 0.006 s part 8 times faster saves about
    5 ms out of 1.07 s. Amdahl's limit is 1 / 0.994 ≈ 1.01. (c) Make the sort
    parallel: each thread sorts its own chunk with `qsort`, then the sorted
    chunks are merged (the merge can also be done in pairs in parallel).
    Now most of the time is parallel and p becomes large.
20. Any four: threads (what you control) must be on the **x-axis** and time on
    the y-axis; add **units** ("time (s)"); add a **title or caption**; use
    several runs per point and show **error bars** or a box plot; add a **key**
    for the colours; choose axis ranges and ticks that fill the graph
    (slide 41).
21. Any two: time the serial and parallel parts separately (to estimate p
    and the Amdahl limit); measure each thread's work or busy time (load
    imbalance); count lock acquisitions or time spent waiting for locks
    (contention); measure with 1, 2, 4, 8 threads and see where the curve
    flattens; check whether memory bandwidth is the bottleneck.

## D. Proof

22. **Work law:** in one time step each of the p processors does at most one
    unit of work, so in Tₚ steps they do at most p · Tₚ units. All T₁ units
    must be done, so p · Tₚ ≥ T₁, i.e. Tₚ ≥ T₁ / p. **Span law:** the strands on
    the longest path form a chain where each must wait for the previous one,
    so even with unlimited processors they take T∞ steps: Tₚ ≥ T∞.
    **Bound:** speedup = T₁ / Tₚ ≤ T₁ / T∞ (using Tₚ ≥ T∞) = parallelism.
23. They answer different questions. Amdahl keeps the **problem size fixed**
    and asks how much faster it gets. Gustafson lets the **problem grow** with
    the machine (more data in the same time) and measures p on the big run,
    where the parallel part dominates. With 10 processors, doing 10 times
    the parallel work in the same time gives a scaled speedup of
    0.1 + 0.9 × 10 = 9.1.
