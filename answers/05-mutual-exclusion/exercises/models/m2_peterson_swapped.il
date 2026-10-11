# ANSWER - Peterson with victim written before flag
shared flag[2] = false
shared victim = 0

thread P(i) for i in 0..1:
    loop:
        noncritical
        victim = i
        flag[i] = true
        await not (flag[1-i] and victim == i)
        critical
        flag[i] = false

# Bad schedule (shortest):
#  P0: victim = 0          P1: victim = 1        P1: flag[1] = true
#  P1: await passes (flag[0] is still false)  -> P1 in CS
#  P0: flag[0] = true;  await passes (victim is 1, not 0) -> P0 in CS too.
# Writing flag first guarantees that whoever writes victim LAST already
# sees (or will see) the other's flag.
