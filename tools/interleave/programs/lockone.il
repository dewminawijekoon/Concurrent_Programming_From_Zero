# L06 LockOne: flag[i] = true; while (flag[j]) {}
shared flag[2] = false

thread P(i) for i in 0..1:
    loop:
        noncritical
        flag[i] = true
        await not flag[1-i]
        critical
        flag[i] = false
