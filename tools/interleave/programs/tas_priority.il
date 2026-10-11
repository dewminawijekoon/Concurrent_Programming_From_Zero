# TAS spin lock on ONE core with a strict-priority scheduler (exam-style Q).
# A and B have HIGH priority, C has LOW priority. A and B start out blocked
# (say, waiting for I/O); C grabs the lock first, then the I/O completes.
# Run:  interleave.py tas_priority.il --priority A=2,B=2,C=1 --no-halt
shared lock = 0, io_done = 0

proc acquire(temp):
    while true:
        atomic:
            temp = lock
            lock = 1
        if temp == 0:
            break

thread A:
    local temp = 0
    await io_done == 1
    acquire(temp)
    critical
    lock = 0

thread B:
    local temp = 0
    await io_done == 1
    acquire(temp)
    critical
    lock = 0

thread C:
    local temp = 0
    acquire(temp)
    io_done = 1         # the I/O for A and B finishes while C holds the lock
    critical
    lock = 0
