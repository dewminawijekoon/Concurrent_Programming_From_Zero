# L06 Filter lock for N = 3 threads (levels 1..N-1)
const N = 3
shared level[N] = 0
shared victim[N] = 0

thread P(i) for i in 0..N-1:
    loop:
        noncritical
        for L in 1..N-1:
            level[i] = L
            victim[L] = i
            await not (any(level[k] >= L for k in range(N) if k != i) and victim[L] == i)
        critical
        level[i] = 0
