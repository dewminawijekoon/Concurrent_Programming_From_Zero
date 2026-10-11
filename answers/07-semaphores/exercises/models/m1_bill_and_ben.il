# ANSWER - Model exercise 1: Bill and Ben (L08 (semaphores) slides 14-18)
#   s = sandwiches made, D = Bill delivers, p = soup made, E = Ben delivers
sem first = 0, second = 0     # first: "soup is ready"; second: "sandwiches are ready"

thread Bill:
    for r in 1..2:
        print s
        second.up()           # SYNCH 1: tell Ben the sandwiches are ready...
        first.down()          # ...then wait for the soup
        print D

thread Ben:
    for r in 1..2:
        print p
        first.up()            # SYNCH 2: tell Bill the soup is ready...
        second.down()         # ...then wait for the sandwiches
        print E

# The explorer finds 24 possible outputs, all following the rule, and no
# stuck run. Each down() can only finish after the other thread's up() of
# the same round, and each up() comes right after that thread's print.
#
# The WRONG version (both threads do down() first) is stuck after "s p" in
# every run: Bill waits for "soup ready", Ben waits for "sandwiches ready",
# and neither will ever do its up(). That is a deadlock.
