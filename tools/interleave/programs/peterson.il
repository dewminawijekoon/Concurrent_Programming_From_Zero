# L06 Peterson's lock (flag/victim form). Thread i's partner is 1-i.
shared flag[2] = false
shared victim = 0

thread P(i) for i in 0..1:
    loop:
        noncritical
        flag[i] = true          # I'm interested
        victim = i              # you go first
        await not (flag[1-i] and victim == i)
        critical
        flag[i] = false
