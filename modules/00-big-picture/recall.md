# 00 · Recall sheet

Cover the right-hand column. Say each answer out loud, or write it on a
blank page. If you hesitate, re-read that part of the [README](README.md).

| Question | What you should be able to say or write |
|---|---|
| What does Moore's law say? | The number of transistors on a chip doubles about every two years. It does not talk about clock speed. |
| Why did we get multicore chips? | Faster clocks made chips too hot, so makers used the extra transistors to add more cores instead. |
| What is "the free ride" and why is it over? | Programs used to get faster on every new computer without changes. Now a program only gets faster if it uses several cores at once. |
| Concurrent vs parallel? | Concurrent: several tasks are in progress at the same time (they may take turns on one core). Parallel: they really run at the same instant on different cores. |
| What is distributed computing? | Separate programs, usually on separate machines, that cooperate by sending messages. |
| What is asynchrony? | A thread can be delayed at any moment for an unknown time (cache miss, page fault, scheduler). So never assume one thread is faster than another. |
| Name Flynn's four classes with an example each. | SISD: an old single-core CPU. SIMD: a GPU or vector instructions. MISD: flight computers that vote on one result. MIMD: a multicore CPU or a cluster. |
| How do you decide between SIMD and MIMD? | Count the instruction streams (program counters). One stream applied to many data items is SIMD. Many independent streams is MIMD. |
| Shared vs distributed memory? | Shared: all processors see one memory and communicate through variables (Pthreads, OpenMP). Distributed: each has private memory and they send messages (MPI). |
| Safety vs liveness (preview)? | Safety: something bad never happens. Liveness: something good eventually happens. |
| What does a PASS from a concurrency test mean? | Only that no problem was seen in the runs tried. It does not prove the code correct. |
| Which two tools find what tests miss? | ThreadSanitizer (`make tsan`) finds unsynchronized accesses while the program runs. The interleaving explorer tries every order of steps of a small model. |
| How do you pass an integer to a new thread and get one back? | Create with `(void *)(long)i`, read with `(int)(long)arg`, return `(void *)(long)value`, collect with `pthread_join(t, &res)`. |

**From memory:** write a program that starts N threads, gives each its
number, and collects a result from each (exercise 1).
