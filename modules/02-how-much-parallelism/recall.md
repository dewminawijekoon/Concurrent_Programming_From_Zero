# 02 · Recall sheet

Cover the right-hand column. Say each answer out loud, or write it on a
blank page. If you hesitate, re-read that part of the [README](README.md).

| Question | What you should be able to say or write |
|---|---|
| Define speedup and efficiency. | Speedup = T₁ / Tₙ (time on 1 processor over time on n). Efficiency = speedup / n, between 0 and 1. |
| Write Amdahl's law and say what p is. | S = 1 / ((1 − p) + p / n). p is the fraction of the running time on one processor that can run in parallel. |
| Derive it in one line. | Time on 1 processor = 1; on n processors the serial part still takes 1 − p and the parallel part takes p / n; speedup is 1 over their sum. |
| What is the limit with unlimited processors? | 1 / (1 − p). With 5% serial, at most 20×. |
| The L02 examples on 10 processors? | 60% → 2.17, 80% → 3.57, 90% → 5.26, 99% → 9.17. |
| How do you answer "more processors or more parallel code?" | Compute both speedups, both limits and both efficiencies, then recommend and explain (usually: raising p raises the ceiling). |
| How do you estimate p from measurements? | From 1-thread phases: p = T_parallel / (T_serial + T_parallel). From a speedup S on n threads: p = (1 − 1/S) / (1 − 1/n). |
| What kills parallelism? | Serial parts, synchronization and contention, communication, thread start-up and termination, load imbalance, bottlenecks such as memory bandwidth. |
| Work, span, parallelism? | Work T₁ = total time on one processor (all strands). Span T∞ = the longest chain of dependent strands. Parallelism = T₁ / T∞ = the best possible speedup. |
| Work law and span law? | Tₚ ≥ T₁ / p (p processors do at most p units per step) and Tₚ ≥ T∞ (the chain runs one step at a time). |
| P-FIB(4) numbers? | Work 17, span 8, parallelism 2.125. |
| Strands of P-FIB(n)? | n ≤ 1: one strand. n ≥ 2: A (spawn), B (call n − 2), C (after sync). work(n) = 3 + work(n − 1) + work(n − 2); span(n) = max(2 + span(n − 1), 3 + span(n − 2)). |
| Latency vs throughput? | Latency: time for one job (user waiting, deadline). Throughput: jobs per unit time (overall use of the machine). |
| What is scalability limited by? | Bottlenecks: CPU, memory bus, network bandwidth, memory size. Remove one and another appears. |
| Sample standard deviation? | s = √(Σ(xᵢ − mean)² / (n − 1)). |
| Confidence interval for the mean? | mean ± z · s / √n, with z = 1.96 for 95%. |
| How many runs for ± r% accuracy? | n = (100 · z · s / (r · mean))², rounded up. Mean 20, s 5, ± 5%, 95% → 96.04 → 97. |
| Graph rules? | Title; labelled axes with units; independent variable (and time) on the x-axis; sensible ticks; a key; error bars for uncertainty. |

**From memory:** derive Amdahl's law; compute work and span of P-FIB(4);
compute the sample size for the slide 36 example.
