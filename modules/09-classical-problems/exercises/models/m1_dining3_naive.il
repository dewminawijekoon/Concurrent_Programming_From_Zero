# Model exercise 1 - three dining philosophers: all pick up their LEFT fork first (deadlocks)
#
# Philosopher Pi needs fork i (on its left) and fork (i+1) % 3 (on its
# right). Forks are semaphores f0, f1, f2 that start at 1. Eating is
# `critical`: with only 3 philosophers, every two of them are neighbours,
# so at most one can eat at a time, and the explorer's mutual exclusion
# check is exactly the rule "no fork is used by two philosophers at once".
#
# TODO: in each philosopher, pick up the LEFT fork (fork i) first, then the
#       right one, eat, then put both down. Then run the explorer:
#   python3 tools/interleave/interleave.py modules/09-classical-problems/exercises/models/m1_dining3_naive.il
# Questions: (i) write down the deadlock trace it shows, in your own words.
#            (ii) which of the four rules does it break, and which hold?
# Test:  make test M=09 T=m1_dining3_naive   (expects: deadlock found, forks never shared)

sem f0 = 1, f1 = 1, f2 = 1

thread P0:
    loop:
        noncritical
        # TODO: pick up f0, then f1; critical; put both down

thread P1:
    loop:
        noncritical
        # TODO: pick up f1, then f2; critical; put both down

thread P2:
    loop:
        noncritical
        # TODO: pick up f2, then f0; critical; put both down
