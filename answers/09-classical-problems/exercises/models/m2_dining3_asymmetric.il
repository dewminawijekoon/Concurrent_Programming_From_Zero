# ANSWER - Model exercise 2: three philosophers, the asymmetric solution
sem f0 = 1, f1 = 1, f2 = 1

thread P0:
    loop:
        noncritical
        f0.down()
        f1.down()
        critical
        f0.up()
        f1.up()

thread P1:
    loop:
        noncritical
        f1.down()
        f2.down()
        critical
        f1.up()
        f2.up()

thread P2:                 # the last philosopher takes fork 0 FIRST (its right fork)
    loop:
        noncritical
        f0.down()
        f2.down()
        critical
        f0.up()
        f2.up()

# (i) Everyone takes the lower-numbered fork first. In a circle of waiting,
#     each philosopher would hold a fork and wait for a HIGHER-numbered one.
#     Going round the circle the numbers would have to keep growing and
#     come back to where they started, which is impossible. Concretely: P0
#     and P2 both want f0 first, so one of them gets nothing at all and
#     cannot be part of a circle.
# (ii) The explorer models a BUSY-WAIT semaphore: down(f) is "wait until
#     f > 0, then take it", and up() wakes nobody. So a philosopher can keep
#     missing the moment a fork is free while its neighbours eat again and
#     again (module 07, section 3.3). With a first-come-first-served
#     (blocked-queue) semaphore that would not happen. Deadlock-freedom is
#     what the slide's solution is for.
