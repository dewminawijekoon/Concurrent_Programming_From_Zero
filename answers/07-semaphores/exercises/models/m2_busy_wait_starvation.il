# ANSWER - Model exercise 2: mutual exclusion with a semaphore (L08 (semaphores) slides 6-13)
sem S = 1

thread P1:
    loop:
        noncritical
        S.down()
        critical
        S.up()

thread P2:
    loop:
        noncritical
        S.down()
        critical
        S.up()

# (i)   Mutual exclusion holds, there is no deadlock and no livelock, and a
#       thread alone gets in at once. Only "no starvation" fails.
# (ii)  Both are right, because they use DIFFERENT semaphores. The explorer's
#       down(S) waits until S > 0 and then decrements: that is the BUSY-WAIT
#       semaphore of slide 11. Its up(S) only adds 1; it does not hand the
#       semaphore to the waiting process. Slide 8's proof uses the
#       BLOCKED-SET semaphore of slide 3/9: up(S) wakes the suspended process
#       directly and S stays 0, so nobody can overtake it. Slide 12 says
#       exactly this: with a busy-wait semaphore, starvation is possible.
# (iii) P1 is waiting at down(S). P2 does up(S) (S = 1), runs its
#       non-critical section, does down(S) again (S = 0) and enters - all
#       before P1 happens to look at S while it is 1. This can repeat
#       forever. P1 is not "ready all the time" (only while S = 1), so even a
#       fair scheduler may never let it in.
