# Model exercise 2 - "x = y + z" on a load-store machine           [L03 slide 41]
#
# The program from the lecture, with y = 0 and z = 0 at the start:
#     P1:  x = y + z;
#     P2:  y = 1;  z = 2;
#
# On a load-store machine, P1's single line is really several steps:
#     load y into a register,  load z into a register,  add,  store into x
# Another thread can run between any two of them.
#
# TODO: write P1 with two local variables a and b (they play the role of
#       registers): one line to load y into a, one line to load z into b,
#       one line to store a + b into x, then `print x`.
#       Write P2 as two lines: y = 1, then z = 2.
#
# PREDICT FIRST: list every final value x can have. Is x == 2 possible?
#
# Check:  python3 tools/interleave/interleave.py modules/01-races-and-interleavings/exercises/models/m2_x_equals_y_plus_z.il --outputs
#         python3 tools/interleave/interleave.py <same file> --why "2"
# Test:   make test M=01 T=m2_x_equals_y_plus_z
#
# Afterwards: swap P1's two loads (load z first, then y). Which value
# disappears, and why?

shared x = 0, y = 0, z = 0

thread P1:
    local a = 0
    local b = 0
    # TODO: load y into a, load z into b, store a + b into x, print x
    skip

thread P2:
    # TODO: y = 1, then z = 2
    skip
