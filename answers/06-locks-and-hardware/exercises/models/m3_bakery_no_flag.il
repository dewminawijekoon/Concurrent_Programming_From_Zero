# ANSWER - model exercise 3: Bakery without flag[]
shared label[2] = 0

thread P(i) for i in 0..1:
    local m = 0
    loop:
        noncritical
        m = label[1-i]                 # read the other's label
        label[i] = m + 1               # take a bigger one (a separate step)
        await not (label[1-i] != 0 and (label[1-i], 1-i) < (label[i], i))
        critical
        label[i] = 0

# Shortest bad order (from the explorer):
#   1. P0 reads label[1] = 0 (m = 0)        P0 is about to write label[0] = 1
#   2. P1 reads label[0] = 0, writes label[1] = 1
#   3. P1 checks: label[0] is 0, so "P0 is not interested" -> P1 enters
#   4. P0 writes label[0] = 1 and checks: (1, 1) < (1, 0)? No -> P0 enters
# Both are inside. P1 looked at P0 while P0 was half-way through taking its
# label. With the flag, P0 raises flag[0] BEFORE reading, so P1 would see
# that P0 is in its doorway; and once P0's label is written it ties with
# P1's, and the tie goes to the lower id.
#
# With the two doorway lines inside one `atomic:` block, the explorer finds
# no problem: nobody can be caught half-way through taking a label, so a
# label of 0 really does mean "not interested". (The real lecture lock keeps
# max() non-atomic and uses the flag instead, which is cheaper than making
# the whole doorway one step.)
