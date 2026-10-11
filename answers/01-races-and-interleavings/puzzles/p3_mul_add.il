# Quiz puzzle: L03 slide 35 (c1 = c1 * c2 and c1 = c1 + c2) on a load-store machine.
shared c1 = 2, c2 = 3, done = 0
thread P1:
    local r = 0
    r = c1
    r = r * c2
    c1 = r
    done++
thread P2:
    local r = 0
    r = c1
    r = r + c2
    c1 = r
    done++
thread W:
    await done == 2
    print c1
