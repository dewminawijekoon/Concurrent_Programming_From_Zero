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
    temp = value
    value = temp + 1
    print temp

# ANSWER: possible outputs are "1 1", "1 2" and "2 1".
# "1 1" means BOTH threads got number 1: that number is tested twice, and
# the counter ends at 2, not 3 (a lost update). Shortest bad order:
#   T0: temp = value (1)   T1: temp = value (1)   T0: value = 2   T1: value = 2
# The fix is to make the read and the write ONE step (an atomic
# getAndIncrement, or a lock around both lines).
