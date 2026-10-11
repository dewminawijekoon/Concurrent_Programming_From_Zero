# Model exercise 2 - the same story with a BLOCKING lock     (L09 slide 3)
#
# Same as m1: one core, H (high priority) wakes up while L (low) holds the
# lock. This time the lock is a semaphore-style mutex: a thread that finds
# it taken is put to sleep by the system (it is NOT ready, so it does not
# use the CPU) and is woken up when the lock is released. This is the
# "sleep and wakeup" fix the slide mentions.
#
#   mutex m        declares the lock (free at the start)
#   m.down()       take it, or sleep until it is free
#   m.up()         release it (wakes a sleeper)
#
# TODO: write H and L using m.down() / m.up() instead of TestAndSet.
#
# PREDICT FIRST: does H still get stuck? Then run:
#   python3 tools/interleave/interleave.py modules/06-locks-and-hardware/exercises/models/m2_priority_block.il --priority H=2,L=1 --no-halt
#
# Test:  make test M=06 T=m2_priority_block

shared woken = 0
mutex m

thread H:
    await woken == 1
    # TODO

thread L:
    # TODO (remember: woken = 1 right after L gets the lock)
    skip
