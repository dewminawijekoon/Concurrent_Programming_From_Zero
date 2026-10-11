# Model exercise 2 - Peterson with the first two lines of lock() swapped
#
#   lock(i):  victim = i;  flag[i] = true;  while (flag[j] && victim == i) {}
#
# Write the model (start from tools/interleave/programs/peterson.il), predict
# what breaks, then run the explorer and read the shortest bad schedule.
# Write that schedule into your notes in your own words: it is a classic
# exam question ("explain why the order of these statements matters").
#
# Test:  make test M=05 T=m2_peterson_swapped

shared flag[2] = false
shared victim = 0

thread P(i) for i in 0..1:
    loop:
        noncritical
        # TODO
        critical
        flag[i] = false
