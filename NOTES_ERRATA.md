# Notes errata

Places where the exported lecture slides (and some lab sheets) are
inconsistent, garbled, or easy to misread. Each module repeats the entries
relevant to it in its "Careful: what the notes say" section. Found another
one? Open an issue or a pull request.

Many slide problems come from the export to text. Pictures, symbols and
exponents were lost, so "1010" means 10¹⁰ and "$ k" means "there exists a k"
(∃k).

## Lectures

| Lecture | Where | What the notes say | What to know |
|---|---|---|---|
| L01 vs L03 | L01 slide 34; L03 slide 43 | L01: "no deadlock" is a *liveness* property. L03: lists "absence of deadlock" under *safety*. | Most textbooks call deadlock-freedom **liveness** ("some thread eventually gets in"). In an exam, say which meaning you use. |
| L01 | producer–consumer pseudocode (slide 59) | the loop that releases the pets and resets the can is labelled "Bob's code"; the loop that stocks the pond is labelled "Alice's code" | The story says the opposite (slide 58: "Alice resets can when pets are fed"): Alice owns the pets (consumer), Bob stocks the food (producer). Go by what the code does. |
| L01 | prime example (slide 21) | the loop tests the previous value of j | each thread ends up testing one number past its limit; harmless, but it explains off-by-one counts (module 01). |
| L02 | Amdahl examples (slides 4–9) | the formula itself is an image; only the answers survive | speedup = 1 / ((1 − p) + p/n). With 10 processors: 60% → 2.17, 80% → 3.57, 90% → 5.26, 99% → 9.17 (module 02). |
| L02 | P-FIB example (slide 26) | "speedup will not be more than 2" | the exact bound is work/span = 17/8 = 2.125. |
| L02 | slide 22 | treats "T₁ = work" and "time of the best serial algorithm" as the same thing | they are not always equal: the parallel algorithm may do extra work. |
| L02 | sample size (slides 35–36) | the formula and the answer are images | n = (100·z·s / (r·mean))². The example (mean 20 s, s = 5, ±5%, z = 1.96) gives 96.04, so **97 runs** (module 02). |
| L03 | interleaving exercise (`x = y + z` vs `y = 1; z = 2`) | no answer is given | if `y` is loaded before `z`: x ∈ {0, 1, 2, 3}. If `z` is loaded first: x ∈ {0, 1, 3} (module 01 checks both with the explorer). |
| L04 | `pthread_create` signature slide | start routine shown as `void*(*)(void)` | correct: `void *(*start_routine)(void *)`. |
| L04 | `pthread_join` (slide 13) | shown taking a `pthread_t *` | it takes the `pthread_t` **by value**: `pthread_join(tid, &ret)`. |
| L04 | compiling (slide 7) | `-lpthread` | prefer `-pthread` (as the labs do); it sets compiler flags as well as linking the library. |
| L04 | matrix–vector (slides 14–17) | assumes the number of threads divides the number of rows | use `first = t*m/p`, `last = (t+1)*m/p` so any thread count works (module 03). |
| L04 | slide 26 | "n = 108" | means 10⁸. |
| L04 | OpenMP part | only `parallel`, `num_threads`, `critical` and the trapezoidal rule | Lab 03 also needs `for`, schedules, `reduction`, `atomic`, `single` and tasks; module 04 adds them [EXTRA]. |
| L05 | slides 7, 13, 16, 21 | the ✓/✗ property tables are blank | module 05 gives the table for each attempt. |
| L06 | Filter (slides 28–31) | `∃` shows as `$`; brackets in the waiting condition are unbalanced; the constructor loop starts at `i = 1` | the condition is "there is another thread k at my level or higher, and I am the victim at this level". In C, initialise **every** array element (module 06). |
| L06 | Bakery (slide 40) | an arrow was lost: "If DA  DB" | it means "if A's doorway finishes before B's doorway starts". "Ties are impossible" is about (label, id) **pairs**; two labels can be equal. |
| L06 | slide 40 notes | "How about the starvation?" has no answer | first-come-first-served implies starvation-freedom. |
| L08 (semaphores) | blocked-set semaphore (slide 13) | starvation for N > 2 "left as an exercise" | module 07 §3.3 gives an example. |
| L08 (semaphores) | Bill & Ben (slides 16–18) | semaphores `first`/`second` with no explanation | all three versions shown are correct; the deadlocking version (both threads `down` first) is not shown (module 07). |
| L08 (semaphores) | infinite buffer invariant (slide 22) | garbled ("#E = 0+tail-head") | number of items = tail − head, which is never negative. |
| L08 (semaphores) | dining philosophers figure (slide 28) | philosopher and fork numbers came out as a jumbled list | fork i lies between philosopher i and philosopher i + 1. |
| L08 (semaphores) | readers–writers (readers' preference) | starvation not discussed | writers can **starve** while readers keep arriving; Lab 02 asks for a writer-preference version (modules 08, 09). |
| L08 (semaphores) | circular buffer | producer/consumer with `elements`/`spaces` only | correct for **one** producer and **one** consumer; with several of each you also need a mutex around the buffer indices (module 07). |
| L08 (semaphores) | sleeping barber (slide 33) | declares a `cutting` semaphore that is never used; no haircut step; the customer does not wait for the haircut to finish | module 09 gives a complete version, including the bug where a new customer "steals" the previous customer's "haircut done" signal. |
| L08 (semaphores) | baboon crossing | problem stated, no solution | module 09 develops one; `demo/baboons.il` checks it with the explorer. |
| L08-TLP | whole deck | header says CS5312 High Performance Computer Architecture | it is background for cache coherence and hardware synchronization (module 06). |
| L08-TLP | slide 31 | test-and-set described as "sets under condition" | the usual meaning: set to true and return the **old** value. |
| L09 | slide 3 | the high/low-priority busy-waiting problem has no name | it is usually called **priority inversion** (module 06). |
| L09 | slides 10 vs 12 | slide 10 says a signalled process joins the service queue (signal-and-continue), but slide 12's bounded buffer uses `if (...) wait` | `if` is only safe under signal-and-exit or signal-and-wait. In Java and Pthreads use `while` (as slide 26 does). |
| L09 | slide 10 | condition queues are FIFO | Pthreads and Java do **not** promise FIFO order; a signal may wake any waiter. |
| L09 | barrier (slide 28) | pictures only, no code | modules 07 and 08 build reusable barriers. |
| L10 | MPI slides | most function signatures are images | module 13 lists them (the exam appendix provides them too). |
| L10 | partitioning (slide 43) | block / cyclic / block-cyclic pictures are lost | module 11 redraws them. |
| L11 | Banker's algorithm and detection | the algorithm steps and matrices are images | module 10 writes them out; `calc/banker` runs `safe`, `request`, `detect` and `schedule`. |
| L11 | slides 24–25 | the prevention summary is an image; the last slide is empty | module 10 summarises prevention per condition. |
| L12 | slide 15 | "with CAS, a wait-free version of any data structure exists" | true (it's a theorem), but the general construction is far too slow for practice. |
| L12 | slide 39 notes | "decrement the permits" for the dequeuer | the dequeuer **increments** them. |
| L12 | slide 61 | "always use signalAll" | a safe rule of thumb; `signal` is fine when each change can help exactly one waiter of a single kind. |
| L12 | slide 71 | `enq()` is missing its final closing brace | the logic is fine. |
| L12 | concurrent data structures | linearizability never defined; no full code for the bounded queue or lock-free `deq()` | module 15 fills these in [EXTRA]. |
| Syllabus | W9 patterns | no lecture covers patterns as a topic (only fragments in L01, L02, L10) | module 11 [EXTRA]. |
| Syllabus | W12 distributed mutual exclusion | no lecture notes cover it | module 14 [EXTRA]. |

## Lab sheets

| Lab | Where | What it says | What to know |
|---|---|---|---|
| Lab 04 | Ex 1 | titled "Traffic Intersection" | the text describes a single-lane bridge. |
| Lab 05 | Ex 3 | the dequeuer's `head == tail` test, and a `volatile int permits` | in C, `volatile` is not atomic and the `head == tail` test races with enqueuers; use an atomic `permits` for both the full and the empty test (module 15). |
| Lab 05 | Ex 4, snippet D | one CAS attempt, then give up | can be read two ways; the intended answer is obstruction-free (module 15 explains both readings). |
| Lab 06 | Ex 6 | "the pattern from the lecture" | no lecture covers patterns; see module 11. |
