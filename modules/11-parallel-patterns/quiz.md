# 11 · Quiz: parallel patterns & design

Answer on paper first, as you would in the exam, then check
[`answers/11-parallel-patterns/quiz-answers.md`](../../answers/11-parallel-patterns/quiz-answers.md).
Marks in brackets show how much detail is expected. Tags: `[L01]` lecture,
`[Lab06]` lab, `[tf]` true/false + justification, `[calc]` calculation,
`[trace]` give an order of events, `[design]` justify a choice, `[code]`.

## A. True or false? Justify briefly. (2 marks each)

1. `[tf][L01]` If every thread gets the same number of items, the threads get the same amount of work.
2. `[tf][Lab06]` `schedule(dynamic)` is always at least as fast as `schedule(static)`.
3. `[tf][Lab06]` A pipeline with 4 equal stages makes each single item finish 4 times sooner.
4. `[tf]` For a purely CPU-bound job on a 4-core machine, 16 threads finish sooner than 4.
5. `[tf][L10]` With cyclic partitioning and more than one thread, two neighbouring items (i and i + 1) never belong to the same thread.
6. `[tf][Lab06]` Work stealing needs no synchronization at all, because every worker has its own deque.
7. `[tf]` A table that all threads read but no thread ever writes can be shared without a lock.

## B. Short answer and calculations

8. `[calc][L10]` (4 marks) 14 items are shared by 4 threads, numbered 0–3. (a) Give each thread's range under block partitioning (the first *n mod p* threads get one extra item). (b) How many items does each thread get under cyclic partitioning? (c) Under block-cyclic partitioning with blocks of 3, which thread owns item 13?

9. `[code]` (5 marks) For each loop, say whether its iterations can be split between threads as written. If not, say why, and whether a fix exists.
   ```c
   (a) for (i = 0; i < n; i++)   c[i] = a[i] + b[i];
   (b) for (i = 1; i < n; i++)   a[i] = a[i] * a[i-1];
   (c) for (i = 0; i < n; i++)   if (a[i] > best) best = a[i];
   (d) for (i = 0; i < n/2; i++) a[2*i] = a[2*i+1];
   (e) for (i = 0; i < n; i++)   count[a[i]]++;        // a histogram
   ```

10. `[calc][Lab06]` (6 marks) Each item passes through three steps that take 4 ms, 12 ms and 4 ms. There are 50 items.
    (a) How long does one thread take if it does every step for every item?
    (b) How long does a 3-stage pipeline take (one thread per stage)?
    (c) What is the pipeline's throughput in items per second?
    (d) The 12 ms stage now runs on 3 threads. New total time and throughput?
    (e) Would a 4th thread for that stage help? Why or why not?

11. `[calc]` (3 marks) A pipeline has 5 stages of 3 s each, and 20 items go through it. How long does it take, and what is the speedup over one machine doing everything? Show the formula.

12. `[design][Lab06]` (4 marks) In a parallel tree sum, a new thread is started for every subtree bigger than a threshold. Explain why a very small threshold (100 nodes) is slow, and why a very large one (a million nodes, on a 2.7-million-node tree) is also slower than a middle value.

13. `[calc][Lab06]` (5 marks) 40 tasks are run on 4 threads. Tasks 0, 1, 2 and 3 take 100 ms each; the other 36 take 1 ms each.
    (a) Static block split (10 consecutive tasks per thread): when does the last thread finish?
    (b) A shared task queue, where a free thread takes the next task in order: when does the last thread finish?
    (c) Speedup of each over one thread. Which pattern would you pick and why?

14. `[design][Lab06]` (4 marks) In work stealing, why does a worker pop tasks from the **back** of its own deque, while thieves steal from the **front**? Give two reasons.

15. `[design][Lab06]` (3 marks) When is work stealing clearly faster than one shared task queue, and when is the difference small?

16. `[trace][Lab06]` (4 marks) A pipeline has 1 stage-1 thread, 3 stage-2 threads and 1 stage-3 thread. When it runs out of items, stage 1 sends just **one** poison pill. Each stage-2 thread forwards a pill it takes and then stops. Describe what happens. How many pills should stage 1 send, and how does stage 3 know when to stop?

17. `[design][Lab06]` (3 marks) In Lab 03 the blur filter was fastest with `schedule(static)`. In Lab 06 the Mandelbrot set is much faster with `schedule(dynamic)`. Explain the difference.

## C. Choose a pattern (Lab 06 Ex 6 style; 3 marks each)

For each problem, name the most suitable pattern (loop parallel, fork/join,
divide & conquer, pipeline, producer/consumer, task queue / thread pool, work
stealing, map-reduce) and justify it in 2–3 sentences.

18. (a) A photo service: every uploaded photo is virus-scanned, then resized, then stored. Resizing takes 5 times longer than the other two steps.
    (b) Find the highest temperature in 50 GB of independent sensor readings.
    (c) A puzzle solver: each partial solution branches into a different, unpredictable number of smaller partial solutions.
    (d) Sharpen an 8000 × 8000 image, where every pixel costs the same.
    (e) A print server receives jobs of very different sizes from many users.

## D. Design justification (exam style)

19. `[design]` (4 marks) A web service runs on 8 cores. Each request uses the CPU for 2 ms and then waits 18 ms for a database. Roughly how many worker threads should it use? Justify.

20. `[design]` (9 marks, 3 each) A log-analysis tool runs on a 16 GB machine.
    (a) The input is 40 GB. How do you process it?
    (b) Each worker thread writes its findings to one shared report file, protected by a mutex. Profiling shows the threads spend most of their time waiting. Why, and what do you change?
    (c) Every record must be looked up in a 200 MB read-only table of known error codes. Where and when should the table be loaded, and does it need a lock?

21. `[code]` (5 marks) Write, in pseudocode, the worker loop of dynamic scheduling with a shared counter and chunk size *c*. Explain why it needs an atomic fetch-and-add.

22. `[code]` (5 marks) Write pseudocode for stage 2 (replicated *k* times) and stage 3 of a pipeline that must emit items in their original order.
