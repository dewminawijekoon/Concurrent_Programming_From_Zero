# Demo: the Bakery lock IS first-come-first-served.        (L06 slides 35-40)
# Each thread prints "start i" before its doorway, "ready i" after its
# doorway (flag raised, label taken), and "CS i" when it enters.
#
#   python3 tools/interleave/interleave.py modules/06-locks-and-hardware/demo/bakery_fcfs.il --why "start 0 ready 0 start 1 ready 1 CS 1 CS 0" --max-output 14
#
# Result: IMPOSSIBLE. If P0 finishes its doorway before P1 starts its
# doorway, P1 reads P0's label and takes a bigger one, so P0 goes first.
# But if the two doorways overlap, either one may go first:
#   ... --why "start 0 start 1 ready 0 ready 1 CS 1 CS 0" --max-output 14   -> possible
shared flag[2] = false
shared label[2] = 0

thread P(i) for i in 0..1:
    local m = 0
    print "start"
    print i
    flag[i] = true
    m = label[1-i]
    label[i] = m + 1
    print "ready"
    print i
    await not (flag[1-i] and (label[1-i], 1-i) < (label[i], i))
    print "CS"
    print i
    flag[i] = false
