# Notes errata

Places where the exported lecture slides are inconsistent, garbled, or easy
to misread. Each module repeats the entries relevant to it in its
"Careful: the notes say…" section. Found another? Open an issue or a PR.

| Lecture | Where | What the notes say | What to know |
|---|---|---|---|
| L01 vs L03 | L01 safety/liveness slides; L03 "correctness" slide | L01: no deadlock is a *liveness* property. L03: lists "absence of deadlock" under *safety*. | Standard usage: deadlock-freedom is **liveness** ("some thread eventually enters"). In an exam, state the definition you use. |
| L01 | producer–consumer "can" pseudocode (slide 59) | the loop that releases the pets and resets the can is labelled "Bob's code", and the loop that stocks the pond is labelled "Alice's code" | The fable has it the other way (slide 58: "Alice resets can when pets are fed"): Alice owns the pets (consumer), Bob stocks the food (producer). Go by what the code does. |
| L02, L06 | many slides | exponents and symbols were lost in the export: "1010" means 10¹⁰, "$ k" means ∃k | |
| L03 | interleaving exercise | `x = y + z` vs `y = 1; z = 2` (load/store) has no answer given | possible results: x ∈ {0, 1, 2, 3} (module 01 derives them). |
| L04 | `pthread_create` signature slide | start routine shown as `void*(*)(void)` | correct: `void *(*start_routine)(void *)`. |
| L05 | slides 7, 13, 16, 21 | the ✓/✗ property tables are blank | module 05's README gives the correct table for each attempt. |
| L08 (semaphores) | sleeping barber | declares a `cutting` semaphore that is never used | harmless; the solution works without it. |
| L08 (semaphores) | readers–writers (readers' preference) | starvation not discussed | writers can **starve** while readers keep arriving; Lab 02 asks for a writer-preference version. |
| L08 (semaphores) | circular buffer | producer/consumer with `elements`/`spaces` only | correct for **one** producer and **one** consumer; with several of each you also need a mutex around the buffer indices. |
| L08 (semaphores) | baboon crossing | the problem is stated but no solution is given | module 09 develops one. |
| L08-TLP | whole deck | header says CS5312 High Performance Computer Architecture | it is background for cache coherence and hardware synchronization (module 06). |
| L10 | MPI slides | most function signatures are images | module 13 lists them (the exam appendix provides them too). |
| L11 | Banker's algorithm | the algorithm steps and matrices are images | module 10 spells out the algorithm and has a calculator. |
| L12 | concurrent data structures | linearizability is never defined, and there is no full code for the bounded queue or lock-free `deq()` | module 15 fills these in [EXTRA]. |
| Syllabus | W12 distributed mutual exclusion | no lecture notes cover it | module 14 [EXTRA]. |
