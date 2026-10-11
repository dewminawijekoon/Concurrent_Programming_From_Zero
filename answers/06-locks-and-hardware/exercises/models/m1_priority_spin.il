# ANSWER - model exercise 1: TAS spin lock, one core, priorities
shared lock = 0, woken = 0

thread H:
    local temp = 0
    await woken == 1              # asleep until the timer fires
    while true:                   # acquire: spin on TestAndSet
        atomic:
            temp = lock
            lock = 1
        if temp == 0:
            break
    critical
    lock = 0                      # release

thread L:
    local temp = 0
    while true:
        atomic:
            temp = lock
            lock = 1
        if temp == 0:
            break
    woken = 1                     # H's timer fires while L holds the lock
    critical
    lock = 0

# With --priority H=2,L=1 the explorer finds a LIVELOCK:
#   L takes the lock, H wakes up. H is ready all the time (spinning is
#   running, not sleeping), and it has the higher priority, so the scheduler
#   never runs L again. L never reaches `lock = 0`, so every TestAndSet by H
#   returns 1. H spins forever, L never runs: nobody makes progress.
# Without --priority (several cores, or a fair scheduler) L eventually runs,
#   releases the lock, and H gets in. The algorithm is fine; the problem is
#   busy-waiting combined with strict priorities on one core.
