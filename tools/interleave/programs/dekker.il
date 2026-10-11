# L05 slide 20, Dekker's algorithm: flags + a turn variable that decides who backs off
shared c1 = 1, c2 = 1, turn = 1

thread P1:
    loop:
        noncritical
        c1 = 0
        while c2 == 0:
            if turn == 2:
                c1 = 1
                await turn == 1
                c1 = 0
        critical
        c1 = 1
        turn = 2

thread P2:
    loop:
        noncritical
        c2 = 0
        while c1 == 0:
            if turn == 1:
                c2 = 1
                await turn == 2
                c2 = 0
        critical
        c2 = 1
        turn = 1
