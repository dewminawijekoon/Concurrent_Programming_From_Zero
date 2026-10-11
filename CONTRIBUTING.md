# Contributing

Thanks for helping classmates learn. Corrections are the most valuable
contribution: a wrong explanation of a concurrency bug is worse than no
explanation.

## Ground rules

1. **Never copy** text, code or figures from the lecture slides, lab sheets
   or past papers. Explain in your own words and cite the source, e.g.
   "L06 slides 23–26" or "Lab 04, Ex 2".
2. **The notes win.** If you think the slides are wrong, say so in a
   "Careful: the notes say…" box and add it to `NOTES_ERRATA.md`, with your
   reasoning.
3. **Mark extra material** with **[EXTRA]**, i.e. anything not in the
   lecture notes.
4. **Every exercise needs a test and an answer**, and `make verify-answers`
   must pass.

## Adding or changing an exercise

Copy the layout in [`modules/_TEMPLATE`](modules/_TEMPLATE). Conventions:

- An exercise `exercises/<track>/<name>.c` is tested by `tests/test_<name>.c`
  (linked together, compiled with `-DTESTING` so the exercise's own `main()`
  is left out) or by `tests/test_<name>.sh` (for whole programs, MPI or
  CUDA).
- The reference answer lives at the **same relative path** under
  `answers/<module>/`.
- Magic comments in the first 30 lines of a test:
  `// BUILD: -fopenmp`, `// REQUIRES: openmp mpi cuda avx2`, `// TIMEOUT: 60`.
- Exercise headers start with the lecture reference, a **PREDICT FIRST**
  question, the task, and the commands to test and run it.
- Tests must be **honest**:
  - A stress test that passes proves nothing; say "no violation observed in
    N runs".
  - Use `harness_watchdog()` so a hang becomes a FAIL.
  - Check *invariants* (via `trace.h` or `harness_cs_monitor`), not just
    final results.
  - Timing never decides PASS/FAIL.
- The **untouched** exercise must compile, and its tests must fail cleanly
  (no crash, no hang): `bash tools/check_exercises.sh`.
- C17 (`-std=gnu17`), `-Wall -Wextra` clean, 4-space indent.

## Quiz questions

Tag each question: lecture or lab (`[L06]`, `[Lab04]`) plus type (`[tf]`,
`[trace]`, `[output]`, `[proof]`, `[design]`, `[calc]`, `[code]`). Give a
mark value, and write the answer at the length a marker expects for those
marks. Verify trace and output answers with `tools/interleave`.

## Before opening a pull request

```bash
make verify-answers          # reference answers pass
bash tools/check_exercises.sh
make interleave-selftest
```
