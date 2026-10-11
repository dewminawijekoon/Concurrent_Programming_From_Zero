# L05 Attempt 4: raise your flag; if the other's flag is up, back off and retry
shared c1 = 1, c2 = 1

thread P1:
    loop:
        noncritical
        c1 = 0
        while c2 == 0:
            c1 = 1
            c1 = 0
        critical
        c1 = 1

thread P2:
    loop:
        noncritical
        c2 = 0
        while c1 == 0:
            c2 = 1
            c2 = 0
        critical
        c2 = 1
