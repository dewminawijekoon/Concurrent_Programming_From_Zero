# Model exercise 2 - three dining philosophers, fixed by ordering the forks
#                    (L11 slide 23: attack circular wait)
#
# Three philosophers sit at a round table with one fork between each pair.
# Philosopher Pk needs fork k and fork k+1 (P2 needs fork 2 and fork 0).
# If every philosopher picks up "my first fork, then my second", the code
#     P0: f0 then f1      P1: f1 then f2      P2: f2 then f0
# can deadlock: each holds one fork and waits for its neighbour's.
#
# TODO: write P0, P1 and P2 so that each picks up its LOWER-numbered fork
# first (forks are mutexes: f.down() picks up, f.up() puts down). Use
# `critical` for eating. Predict, then run:
#   python3 tools/interleave/interleave.py modules/10-deadlocks/exercises/models/m2_philosophers.il
# (The explorer may report STARVATION: its mutexes do not serve waiters in
# arrival order. That is a different problem from deadlock.)
#
# Test:  make test M=10 T=m2_philosophers

mutex f0, f1, f2

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
