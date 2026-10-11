# L06 LockTwo: victim = i; while (victim == i) {}
shared victim = 0

thread P(i) for i in 0..1:
    loop:
        noncritical
        victim = i
        await victim != i
        critical
