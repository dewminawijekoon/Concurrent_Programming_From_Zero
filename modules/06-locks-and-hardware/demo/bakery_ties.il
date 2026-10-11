# Demo: why Bakery breaks ties with the thread id.          (L06 slides 35-40)
# Two threads can take the SAME label (both read the other's label before
# either writes its own). This version compares labels only:
#
#   python3 tools/interleave/interleave.py modules/06-locks-and-hardware/demo/bakery_ties.il --bound 4
#   -> Mutual exclusion VIOLATED: label = [1, 1]; neither label is smaller, both go in.
#
# Now change `<` to `<=` in the await line and run it again:
#   -> DEADLOCK: label = [1, 1]; each thread waits for the other.
# The real lock compares (label, id) pairs, which are never equal.
shared flag[2] = false
shared label[2] = 0

thread P(i) for i in 0..1:
    local m = 0
    loop:
        noncritical
        flag[i] = true
        m = label[1-i]
        label[i] = m + 1
        await not (flag[1-i] and label[1-i] < label[i])
        critical
        flag[i] = false
