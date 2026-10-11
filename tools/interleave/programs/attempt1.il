# L05 Attempt 1: one shared `turn` variable (strict alternation)
shared turn = 1

thread P1:
    loop:
        noncritical
        await turn == 1
        critical
        turn = 2

thread P2:
    loop:
        noncritical
        await turn == 2
        critical
        turn = 1
