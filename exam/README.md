# Exam preparation

The final exam is **2 hours, closed book**. In the past paper we analysed
(CS4532, July 2019) it had **4 questions × 25 marks**, all compulsory. An
appendix listed the MPI function signatures and a few series formulas. You
don't need to memorise MPI signatures, but you must know how to *use* them.

Past papers belong to the university and are not reproduced here. This page
maps the **types** of question asked to the modules that train them. Our own
exam-style practice questions, with model answers, will be added below.

## Question types and where to practise them

| Question type (as asked) | Marks seen | Practise in |
|---|---|---|
| **True/False with justification**, e.g. "do independent tasks need synchronization?", "is this loop parallelizable as-is?", "is preventing hold-and-wait always appropriate?", "monitors vs semaphores: which is safer?" | 2 each | every quiz, section A |
| **Lock code analysis**: a test-and-set spin lock on a single core with priorities (what goes wrong?), and what happens if test-and-set is not atomic | 2 + 2 | 06 · `tools/interleave` with `--priority` |
| **"Is this output possible?"** for two threads printing letters while updating shared counters; also "can it print … forever?" | 2–3 each | 01, 07 · `interleave --why`, `--infinite` |
| **Semaphore patterns with timing**: a lightswitch guarding a resource, threads starting at given times | 3 + 3 | 07, 09 · `programs/lightswitch.il` |
| **Pipeline design**: multi-stage processing, time on 1 machine vs a pipeline over 3 machines | 2 + 2 + 2 | 11 |
| **System design justification**: how many threads for an N-core machine, a memory budget smaller than the input, contention on a shared output file, loading read-only data up front | 3–4 each | 11 |
| **Deadlock with multiple resource instances**: allocation → still-needed matrix, a schedule that deadlocks, a schedule that completes | 2 + 2 + 2 | 10 · `calc/banker` |
| **Loop-carried dependence**: remove it (closed form or prefix sum), then outline the MPI program | 4 + 6 | 04, 13 |
| **Amdahl's law**: compare "more processors" against "more parallel fraction" and recommend one | 3 + 3 + 3 | 02 · `calc/amdahl` |
| **SIMD / AVX**: what it is and how it raises parallelism | 6 | 12 [EXTRA] |
| **Load balancing** for an irregular workload (prime-based series) | 5 | 11, 16 |
| **MPI pseudocode outline**, result on rank 0, key functions and parameters | 10 | 13, 16 |
| **CUDA solution outline** for the same problem | 10 | 12, 16 |

## How to use this before the exam

1. Do every module's `recall.md` from a blank page.
2. For each row above, answer one question from the linked module's quiz
   **on paper, against the clock**: about 1 minute per mark.
3. Check trace and output answers with `tools/interleave`, and calculations
   with the module calculators.
4. Compare your answer with the answers in `answers/`: did you give the
   *justification*, not just the verdict?

## Practice papers

*Coming soon:* full mock papers in the same format, written from scratch
for this repo, with model answers.
