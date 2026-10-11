# Demo: the Filter lock is NOT first-come-first-served.    (L06 slide 33)
# Filter for n = 3, but only P0 and P1 try (P2 never asks for the lock).
# Each thread prints its id as soon as it has written level[i] = 1 (it has
# "arrived"), and prints CS and its id when it gets into the critical section.
#
#   python3 tools/interleave/interleave.py modules/06-locks-and-hardware/demo/filter_overtake.il --why "0 1 CS 1 CS 0"
#
# Result: POSSIBLE. P0 arrives first, P1 arrives later, yet P1 enters first.
# How: P1 writes victim[1] = 1, then P0 writes victim[1] = 0. P0 is now the
# victim of level 1 and must wait, so P1 overtakes it.
# Compare with bakery_fcfs.il, where the matching order is impossible.
const N = 3
shared level[N] = 0
shared victim[N] = 0

thread P(i) for i in 0..N-1:
    local L = 0
    if i == 2:
        return
    level[i] = 1
    print i
    for L in 1..N-1:
        level[i] = L
        victim[L] = i
        await not (any(level[k] >= L for k in range(N) if k != i) and victim[L] == i)
    print "CS"
    print i
    level[i] = 0
