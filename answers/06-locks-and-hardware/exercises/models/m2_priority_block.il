# ANSWER - model exercise 2: the same story with a blocking lock
shared woken = 0
mutex m

thread H:
    await woken == 1
    m.down()                      # lock taken: H goes to SLEEP (not ready)
    critical
    m.up()

thread L:
    m.down()
    woken = 1
    critical
    m.up()                        # releases the lock and wakes H

# No livelock and no deadlock: when H finds the lock taken it is put to
# sleep, so it is no longer "ready". The only ready thread is L, so the
# scheduler runs L even though its priority is lower. L releases the lock,
# H wakes up, takes it, and both finish.
