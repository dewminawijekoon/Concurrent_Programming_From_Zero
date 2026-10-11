# L05 Attempt 3: raise your own flag, THEN look at the other's
shared c1 = 1, c2 = 1

thread P1:
    loop:
        noncritical
        c1 = 0
        await c2 == 1
        critical
        c1 = 1

thread P2:
    loop:
        noncritical
        c2 = 0
        await c1 == 1
        critical
        c2 = 1
