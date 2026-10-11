# Model exercise 2 - mutual exclusion with a semaphore (L08 (semaphores) slides 6-13)
#
# TODO: write the slide 6 solution for two processes: one semaphore S that
#       starts at 1; each process does noncritical, down(S), critical, up(S),
#       forever (use `loop:`).
#
# Run it:
#   python3 tools/interleave/interleave.py modules/07-semaphores/exercises/models/m2_busy_wait_starvation.il
#
# Then answer:
#   (i)   Which of the four rules (module 05) does the explorer say hold?
#   (ii)  Slide 8 proves "no starvation for N = 2", but the explorer finds
#         starvation. Who is right? Hint: in this explorer, down(S) is
#         "wait until S > 0, then S = S - 1", and up(S) is just "S = S + 1".
#         Which of the semaphore definitions on slides 9-11 is that?
#   (iii) Describe the starvation loop in your own words.
#
# Test:  make test M=07 T=m2_busy_wait_starvation

thread P1:
    loop:
        noncritical
        # TODO

thread P2:
    loop:
        noncritical
        # TODO
