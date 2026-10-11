# ANSWER - Model exercise 1: three philosophers, everyone takes the left fork first
sem f0 = 1, f1 = 1, f2 = 1

thread P0:
    loop:
        noncritical
        f0.down()
        f1.down()
        critical
        f0.up()
        f1.up()

thread P1:
    loop:
        noncritical
        f1.down()
        f2.down()
        critical
        f1.up()
        f2.up()

thread P2:
    loop:
        noncritical
        f2.down()
        f0.down()
        critical
        f2.up()
        f0.up()

# (i) P0 takes f0, P1 takes f1, P2 takes f2. Now P0 waits for f1 (held by
#     P1), P1 waits for f2 (held by P2), P2 waits for f0 (held by P0). Each
#     waits for the next one around the table: a circle, so nobody moves.
# (ii) Mutual exclusion holds (a fork semaphore is a binary semaphore, so
#     no fork is used twice). "No deadlock" fails, as shown. Starvation is
#     also reported (once there is a deadlock, the other rules matter less).
