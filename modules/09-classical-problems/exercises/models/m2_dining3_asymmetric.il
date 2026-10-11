# Model exercise 2 - three dining philosophers: the asymmetric fix of L08 slide 28
#
# Philosopher Pi needs fork i (on its left) and fork (i+1) % 3 (on its
# right). Forks are semaphores f0, f1, f2 that start at 1. Eating is
# `critical`: with only 3 philosophers, every two of them are neighbours,
# so at most one can eat at a time, and the explorer's mutual exclusion
# check is exactly the rule "no fork is used by two philosophers at once".
#
# TODO: copy your m1 model and change ONE philosopher so that every
#       philosopher picks up the LOWER-numbered of its two forks first.
#   python3 tools/interleave/interleave.py modules/09-classical-problems/exercises/models/m2_dining3_asymmetric.il
# Questions: (i) why can the circle of waiting no longer close?
#            (ii) the explorer still reports STARVATION. Why? (Hint: module 07,
#                 model m2 - which kind of semaphore does the explorer use?)
# Test:  make test M=09 T=m2_dining3_asymmetric   (expects: no deadlock, forks never shared)

sem f0 = 1, f1 = 1, f2 = 1

thread P0:
    loop:
        noncritical
        # TODO

thread P1:
    loop:
        noncritical
        # TODO

thread P2:
    loop:
        noncritical
        # TODO
