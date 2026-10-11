# Model exercise 1 - a CAS-loop counter, checked by the explorer   [L12 slide 16; Lab 05 Ex 4]
#
# Two threads each add 1 to `counter` ONCE. A third thread waits until both
# are done and prints the result. Write the increment as a CAS loop:
#     repeat: old = counter;  CAS(counter, old, old + 1)  until it succeeds
# The CAS must be ONE step, so write it as an `atomic:` block that compares
# and (only if equal) writes. Then run:
#     python3 tools/interleave/interleave.py <this file> --outputs
# A correct counter can only ever print 2.
# Then try the "give up after one failed CAS" version (Lab 05 snippet D):
# which outputs become possible, and why?
#
# Test:  make test M=15 T=m1_cas_counter

shared counter = 0
shared done = 0

thread T(i) for i in 0..1:
    local old = 0
    local ok = false
    # TODO: the CAS loop (one atomic: block for the CAS itself)
    skip
    done++

thread Check:
    await done == 2
    print counter
