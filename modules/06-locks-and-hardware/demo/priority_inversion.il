# Demo [EXTRA]: priority inversion with a BLOCKING lock.
# One core, three priorities: H (high) > M (medium) > L (low).
# L takes the lock. Then H and M wake up. H finds the lock taken and sleeps.
# M does not need the lock at all; it just computes for a long time.
#
#   python3 tools/interleave/interleave.py modules/06-locks-and-hardware/demo/priority_inversion.il --priority H=3,M=2,L=1 --no-halt
#
# Result: H never gets in. M is the highest-priority READY thread, so it
# runs forever; L (which must release the lock for H) never gets the CPU.
# In effect H waits for M, a LOWER priority thread: the priorities are
# "inverted". The usual fix is priority inheritance: while L holds a lock
# that H wants, L temporarily runs with H's priority.
shared woken = 0, work = 0
mutex m

thread H:
    await woken == 1
    m.down()
    critical
    m.up()

thread M:
    await woken == 1
    loop:
        work = 1 - work          # long computation, never needs the lock

thread L:
    m.down()
    woken = 1                    # H and M wake up while L holds the lock
    critical
    m.up()
