# Model exercise 1 - a test-and-set spin lock on ONE core with priorities
#                    (exam question type; L09 slide 3)
#
# Story: one CPU core. The scheduler always runs the highest-priority thread
# that is ready; a lower-priority thread runs only when no higher one can.
#   H has HIGH priority. At the start it is asleep (waiting for a timer).
#   L has LOW priority. It takes the lock; while L holds it, H's timer fires.
# Both use the same spin lock:
#     acquire:  repeat { temp = TestAndSet(lock) } until temp == 0
#     release:  lock = 0
#
# TODO: write threads H and L below. Use an `atomic:` block for TestAndSet
# (see tools/interleave/programs/tas_lock.il). Model "H wakes up while L
# holds the lock" with the shared variable `woken`: H starts with
# `await woken == 1`, and L sets `woken = 1` right after it gets the lock.
#
# PREDICT FIRST: what happens once H wakes up? Then run:
#   python3 tools/interleave/interleave.py modules/06-locks-and-hardware/exercises/models/m1_priority_spin.il --priority H=2,L=1 --no-halt
# and also WITHOUT --priority (several cores / a fair scheduler). Explain
# the difference in two sentences.
#
# Test:  make test M=06 T=m1_priority_spin

shared lock = 0, woken = 0

thread H:
    local temp = 0
    await woken == 1
    # TODO: acquire, critical, release

thread L:
    local temp = 0
    # TODO: acquire, then woken = 1, then critical, release
