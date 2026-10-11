# Model exercise 3 - why does the Bakery lock need the flag?   (L06 slides 36-40)
#
# A student simplifies the 2-thread Bakery lock. "A label of 0 already means
# 'not interested', so I don't need flag[]." Their version:
#
#   lock(i):   m = label[j]                 # read the other's label (one step)
#              label[i] = m + 1             # take a bigger one     (another step)
#              wait while label[j] != 0 and (label[j], j) < (label[i], i)
#   unlock(i): label[i] = 0                 # "not interested" again
#
# (j is the other thread, written 1-i in the explorer.)
#
# TODO: write this version for the explorer (no flag[] at all), predict what
# breaks, then run:
#   python3 tools/interleave/interleave.py modules/06-locks-and-hardware/exercises/models/m3_bakery_no_flag.il --bound 4
# Read the shortest bad order of steps and explain it in your own words.
# Then: put the two doorway lines inside ONE `atomic:` block (so taking the
# label is a single step) and run it again. What changes, and why?
#
# Test:  make test M=06 T=m3_bakery_no_flag   (checks the version WITHOUT atomic)

shared label[2] = 0

thread P(i) for i in 0..1:
    local m = 0
    loop:
        noncritical
        # TODO: the doorway (two lines) and the wait
        critical
        label[i] = 0
