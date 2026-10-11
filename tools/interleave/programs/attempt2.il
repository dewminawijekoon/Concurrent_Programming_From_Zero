# L05 Attempt 2: look at the other's flag, THEN raise your own  (c = 0 means "I want in")
shared c1 = 1, c2 = 1

thread P1:
    loop:
        noncritical
        await c2 == 1
        c1 = 0
        critical
        c1 = 1

thread P2:
    loop:
        noncritical
        await c1 == 1
        c2 = 0
        critical
        c2 = 1
