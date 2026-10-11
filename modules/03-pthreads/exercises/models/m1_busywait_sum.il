# Model exercise 1 - the global sum with busy-waiting      [L04 slides 21-26]
#
# Three threads each add a partial sum to a shared sum: thread T(i) adds i+1,
# so the final sum must be 1 + 2 + 3 = 6. Adding is split into a read and a
# write (tmp = sum; sum = tmp + ...), because that is what the CPU does.
#
# First run it as it is and look at the outputs:
#   python3 tools/interleave/interleave.py modules/03-pthreads/exercises/models/m1_busywait_sum.il --outputs
# You will see lost updates: sums that end below 6.
#
# TODO: protect the update with a busy-wait on `turn`, the way L04 does it:
#   - before reading sum, wait until it is your turn (use `await`);
#   - after printing, give the turn to the next thread: (turn + 1) % N.
#
# Then answer:
#   (a) What outputs are possible now? Why only that one?
#   (b) Thread T2 is ready first but T0 is slow. What does T2 do meanwhile,
#       and why is that a waste on a real computer?
#   (c) Suppose you have 8 threads but only 2 CPU cores. Why can the
#       busy-wait version become very slow? (L04 slide 26)
#
# Test:  make test M=03 T=m1_busywait_sum

const N = 3
shared sum = 0, turn = 0

thread T(i) for i in 0..N-1:
    local tmp = 0
    tmp = sum
    sum = tmp + i + 1
    print sum
