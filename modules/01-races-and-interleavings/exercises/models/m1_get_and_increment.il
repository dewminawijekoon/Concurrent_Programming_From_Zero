# Model exercise 1 - the shared counter's getAndIncrement()      [L01 slides 20-24]
#
# In the prime example, every thread asks a shared counter for "the next
# number to test" with getAndIncrement(). Written as one line it looks safe,
# but it is really three steps (L01 slide 23):
#     temp  = value        (read)
#     value = temp + 1     (write)
#     return temp
#
# TODO: write thread T(i) so that it does those steps on separate lines,
#       using a local variable temp, and then `print temp` (the number it got).
#
# PREDICT FIRST: two threads each call getAndIncrement() once, starting
# from value = 1. Which pairs of numbers can they print? Can both get 1?
#
# Check:  python3 tools/interleave/interleave.py modules/01-races-and-interleavings/exercises/models/m1_get_and_increment.il --outputs
#         python3 tools/interleave/interleave.py <same file> --why "1 1"
# Test:   make test M=01 T=m1_get_and_increment

shared value = 1

thread T(i) for i in 0..1:
    local temp = 0
    # TODO: the read, the write, and print temp - one step per line
    skip
