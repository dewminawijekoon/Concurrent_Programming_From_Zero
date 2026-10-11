# ANSWER - model exercise 1 (L06 slide 14, second case)
mutex L1, L2

thread T1:
    loop:
        noncritical
        L1.down()
        L2.down()
        critical
        L2.up()
        L1.up()

thread T2:
    loop:
        noncritical
        L2.down()
        L1.down()
        critical
        L1.up()
        L2.up()

# Shortest deadlock (explorer): T1 takes L1, T2 takes L2, then T1 waits for
# L2 and T2 waits for L1. Conditions: each lock is held by one thread at a
# time (mutual exclusion); each thread keeps one lock while asking for the
# other (hold and wait); a mutex cannot be taken away (no preemption); T1
# waits for T2 and T2 waits for T1 (circular wait).
# First case (both lock L1 then L2): no deadlock. Whoever gets L1 first can
# always get L2 too, because the other thread is still waiting for L1 and
# holds nothing.
