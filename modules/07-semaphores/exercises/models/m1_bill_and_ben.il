# Model exercise 1 - Bill and Ben (L08 (semaphores) slides 14-18)
#
# Two rounds of the cafe. The prints mean:
#   s = Bill has made the sandwiches     D = Bill delivers a meal
#   p = Ben has made the soup            E = Ben delivers a meal
# Rule: in each round, Bill may print D only after Ben's p of that round,
# and Ben may print E only after Bill's s of that round.
#
# TODO 1: declare two semaphores (and their starting values) on the line below.
# TODO 2: write SYNCH 1 (Bill) and SYNCH 2 (Ben) with down() and up().
# Then try the WRONG version too: put down() before up() in BOTH threads.
# What does the explorer say, and why?
#
#   python3 tools/interleave/interleave.py modules/07-semaphores/exercises/models/m1_bill_and_ben.il --outputs --show 100
# Test:  make test M=07 T=m1_bill_and_ben
#        (checks: no run gets stuck, and every possible output follows the rule)

# TODO 1 here

thread Bill:
    for r in 1..2:
        print s
        # TODO 2: SYNCH 1
        print D

thread Ben:
    for r in 1..2:
        print p
        # TODO 2: SYNCH 2
        print E
