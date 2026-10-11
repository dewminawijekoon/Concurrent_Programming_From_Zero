# Model exercise 1 - the proposed solution on L05 slides 24-25
#
# Write the two processes P and Q from the slide in this file, line by line
# (see tools/interleave/README.md for the language). Use `noncritical` for
# a1/a2, `await` for the busy-wait loops b1/b2, and `critical` for d1/d2.
#
# Then answer, and check with the explorer:
#   (i)  Does the algorithm work correctly in the absence of contention?
#   (ii) Can the algorithm deadlock?
#   (iii) Not asked on the slide, but the real question: is it a correct lock?
#
#   python3 tools/interleave/interleave.py modules/05-mutual-exclusion/exercises/models/m1_slide24.il
# Test:  make test M=05 T=m1_slide24
#        (checks that your model behaves like the slide's algorithm)

shared p = 1, q = 1

thread P:
    loop:
        noncritical
        # TODO b1, c1, d1, e1

thread Q:
    loop:
        noncritical
        # TODO b2, c2, d2, e2
