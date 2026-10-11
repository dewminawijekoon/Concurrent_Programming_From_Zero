# ANSWER - model exercise 2: three philosophers, forks taken in number order
mutex f0, f1, f2

thread P0:
    loop:
        noncritical
        f0.down()
        f1.down()
        critical
        f1.up()
        f0.up()

thread P1:
    loop:
        noncritical
        f1.down()
        f2.down()
        critical
        f2.up()
        f1.up()

thread P2:
    loop:
        noncritical
        f0.down()        # lower number first: f0 before f2
        f2.down()
        critical
        f2.up()
        f0.up()

# No deadlock: a cycle would need someone holding a higher fork while
# waiting for a lower one, and nobody ever does that. Whoever holds the
# highest-numbered fork that is taken can always finish eating.
# The explorer may still report STARVATION: its mutexes let any waiter win,
# so one philosopher can lose every race. That is unfairness, not deadlock.
