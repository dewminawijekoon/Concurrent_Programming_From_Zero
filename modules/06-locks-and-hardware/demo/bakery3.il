# Demo: the Bakery lock for 3 threads, with a NON-atomic max().
# The doorway reads the labels one at a time (a loop over k), exactly like
# the C exercise. Labels grow forever, so we cut the search off with --bound.
#
#   python3 tools/interleave/interleave.py modules/06-locks-and-hardware/demo/bakery3.il --bound 4
#
# Result: all five checks OK (it takes a few seconds: about 100 000 situations).
# Two threads can get the same label here; the thread id breaks the tie.
const N = 3
shared flag[N] = false
shared label[N] = 0

thread P(i) for i in 0..N-1:
    local m = 0
    local k = 0
    loop:
        noncritical
        flag[i] = true
        m = 0
        for k in 0..N-1:
            if label[k] > m:
                m = label[k]
        label[i] = m + 1
        await not any(flag[k] and (label[k], k) < (label[i], i) for k in range(N) if k != i)
        critical
        flag[i] = false
