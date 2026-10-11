# ANSWER - model of L05 slides 24-25 (structurally the same as Attempt 2)
shared p = 1, q = 1

thread P:
    loop:
        noncritical          # a1
        await q == 1         # b1: while (q != 1) {}
        p = 0                # c1
        critical             # d1
        p = 1                # e1

thread Q:
    loop:
        noncritical          # a2
        await p == 1         # b2: while (p != 1) {}
        q = 0                # c2
        critical             # d2
        q = 1                # e2

# (i)  Yes. If Q stays in its non-critical section, q stays 1, so P passes b1
#      every time; symmetric for Q.
# (ii) No. To deadlock both must wait at b forever, i.e. p == 0 and q == 0.
#      But p == 0 only while P is between c1 and e1, i.e. PAST b1 - so P is
#      not waiting. Contradiction.
# (iii) It is NOT a correct lock: P passes b1 (q==1), Q passes b2 (p==1),
#      then both set their flags and both enter - the check and the set are
#      two separate steps (same flaw as Attempt 2).
