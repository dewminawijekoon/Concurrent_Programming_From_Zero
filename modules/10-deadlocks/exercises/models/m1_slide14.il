# Model exercise 1 - "Will this combination of 2 locks be deadlock free?"
#                    (L06 slide 14, second case)
#
#   thread T1: L1.lock(); L2.lock(); critical; unlock both
#   thread T2: L2.lock(); L1.lock(); critical; unlock both
#
# TODO: write T1 and T2 with the mutexes below (m.down() = lock,
# m.up() = unlock). Predict the answer, then run:
#   python3 tools/interleave/interleave.py modules/10-deadlocks/exercises/models/m1_slide14.il
# Write down the shortest deadlocking order of steps the explorer shows, and
# say which of the four deadlock conditions each part of it creates.
# Then change T2 to lock L1 first (the slide's first case) and run it again.
#
# Test:  make test M=10 T=m1_slide14   (checks the opposite-order version)

mutex L1, L2

thread T1:
    loop:
        noncritical
        # TODO

thread T2:
    loop:
        noncritical
        # TODO
