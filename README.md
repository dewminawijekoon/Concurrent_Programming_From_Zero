# Concurrent Programming From Zero

A hands-on, **coding-first** companion for **CS4533 Concurrent Programming**
(Department of Computer Science & Engineering, University of Moratuwa).

You learn each idea by building it, breaking it, and checking it. The goal is
to understand **what happens, how and why**: well enough to write it from
memory in a closed-book exam, and well enough to make the labs easy.

> **Unofficial.** This is a student-made practice resource. The course
> staff have not reviewed or endorsed it. If anything here disagrees with
> the lecture notes, **the notes win**; please open an issue. Follow the
> course's plagiarism policy: use this to learn, then write your own lab
> and homework solutions.

## What's inside

Every module in [`modules/`](modules) has the same parts:

| File | What it is |
|---|---|
| `README.md` | the lesson: **problem (why) → idea (what) → mechanism (how) → why it works**, exam traps, and corrections to the slides |
| `exercises/concept/` | C exercises with `TODO`s: warm-up → core → challenge → **from memory** |
| `exercises/lab/` | stepping stones to the matching lab, in the same style |
| `exercises/models/` | small models for the **interleaving explorer** |
| `tests/` | self-tests: `make test M=05` says PASS / FAIL / SKIP |
| `quiz.md` | exam-style questions: true/false + justify, "is this output possible?", traces, design |
| `recall.md` | one page: what you must be able to reproduce from a blank page |
| `break_it/` | demos that show a bug actually happening |

Reference solutions and quiz answers are in [`answers/`](answers). Try first.

Two tools come with the repo:

- [`tools/interleave`](tools/interleave): tries **every** interleaving of a
  small program. Use it for "can this deadlock?", "is output `B B A`
  possible?", "does this lock starve?".
- Calculators (Amdahl, work/span, Banker's algorithm, …) inside the modules
  that use them. They print a full worked solution.

## Quick start

**Option 1: Dev Container (any OS, recommended).** Open the repo in VS Code
with the *Dev Containers* extension, or in GitHub Codespaces. Everything is
installed for you.

**Option 2: Ubuntu / WSL2.**
```bash
bash setup/install.sh        # gcc, OpenMP, OpenMPI, gdb, valgrind (+ CUDA if you have an NVIDIA GPU)
```

**Option 3: anything else.** You need `gcc` with OpenMP, `make`, Python 3,
and OpenMPI for the MPI modules. On **macOS**, Apple's clang has no OpenMP,
and macOS lacks unnamed POSIX semaphores and `pthread_barrier_t`. Use the
dev container. No NVIDIA GPU? Module 12 shows how to run the CUDA exercises
free on Google Colab.

Then:

```bash
make check                        # what can this machine run?
make test M=05                    # test your module-05 exercises
make test M=05 T=ex1_peterson     # one exercise
make run  M=05 EX=ex1_peterson    # build and run it on its own
make tsan M=03                    # same tests under ThreadSanitizer (finds data races)
make progress                     # how far have you got?
python3 tools/interleave/interleave.py tools/interleave/programs/peterson.il
```

## Modules

The order follows **learning dependencies**. The table also shows where each
module sits in the course schedule and which lab it prepares you for.
★ = heavily examined.

| # | Module | Lectures | Syllabus week | Lab |
|---|---|---|---|---|
| 00 | [Big picture & setup](modules/00-big-picture) | L01 | W1 | |
| 01 | [Races & interleavings](modules/01-races-and-interleavings) ★ | L01, L03 | W1 | Lab 1 |
| 02 | [How much parallelism? (Amdahl, work/span, measuring)](modules/02-how-much-parallelism) ★ | L02 | W2 | Lab 1 |
| 03 | [Pthreads](modules/03-pthreads) | L04 | W3 | Lab 1 |
| 04 | [OpenMP](modules/04-openmp) | L04 | W4 | Lab 3 |
| 05 | [Mutual exclusion from scratch](modules/05-mutual-exclusion) ★ | L05, L06 | W6 | |
| 06 | [N-thread locks & the hardware](modules/06-locks-and-hardware) ★ | L06, L08-TLP | W6, W8 | |
| 07 | [Semaphores & synchronization patterns](modules/07-semaphores) ★ | L08 | W7 | Lab 4 |
| 08 | [Monitors & condition variables](modules/08-monitors-and-condition-variables) ★ | L09 | W2, W7 | Lab 2 |
| 09 | [Classical problems](modules/09-classical-problems) ★ | L08, L03 | W10 | Lab 4 |
| 10 | [Deadlocks](modules/10-deadlocks) ★ | L11, L06 | W11 | |
| 11 | [Parallel patterns & design](modules/11-parallel-patterns) ★ | L01, L02, L10 | W9 | Lab 6 |
| 12 | [SIMD & GPUs (CUDA)](modules/12-simd-and-gpu) | L07 | W5 | |
| 13 | [MPI](modules/13-mpi) | L10 | W13 | |
| 14 | [Distributed mutual exclusion](modules/14-distributed-mutual-exclusion) | (not in the notes) | W12 | |
| 15 | [Concurrent data structures](modules/15-concurrent-data-structures) ★ | L12 | W8, W14 | Lab 5 |
| 16 | [Capstone: one problem, four paradigms](modules/16-capstone) ★ | all | | |

**Following the lectures week by week?** W1 → 00, 01 · W2 → 02, 08 · W3 →
03 · W4 → 04 · W5 → 12 · W6 → 05, 06 · W7 → 07, 08 · W8 → 06, 15 · W9 → 11 ·
W10 → 09 · W11 → 10 · W12 → 14 · W13 → 13 · W14 → 15.

Material marked **[EXTRA]** goes beyond the lecture notes. It fills gaps in
the slides or covers syllabus topics the slides don't (e.g. distributed
mutual exclusion, SIMD). It can still be useful in the exam, but study the
core material first.

## Exam preparation

[`exam/`](exam) maps past-paper question *types* to modules and adds our own
exam-style questions with model answers. Most module quizzes use the same
formats.

## Contributing

Found a mistake, or want to add a question or exercise? See
[CONTRIBUTING.md](CONTRIBUTING.md). Known problems in the lecture slides are
collected in [NOTES_ERRATA.md](NOTES_ERRATA.md).

## License

Code is under the MIT license; text is under CC BY 4.0. See
[LICENSE](LICENSE). The lecture slides, lab sheets and past papers belong
to their authors and are **not** included in this repository.
