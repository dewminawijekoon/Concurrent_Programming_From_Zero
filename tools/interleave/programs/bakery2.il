# L06 Lamport's Bakery lock, 2 threads. Labels grow forever, so run with --bound.
# Taking a label reads the other labels one at a time (max() is NOT atomic here).
const N = 2
shared flag[N] = false
shared label[N] = 0

thread P(i) for i in 0..N-1:
    local m = 0
    loop:
        noncritical
        flag[i] = true
        m = label[1-i]                      # read the other's label
        label[i] = m + 1                    # take a bigger one
        await not (flag[1-i] and (label[1-i], 1-i) < (label[i], i))
        critical
        flag[i] = false
