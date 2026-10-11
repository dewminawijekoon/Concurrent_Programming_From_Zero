# TestAndSet spin lock (2019 exam Q1b). TestAndSet copies lock into temp and
# sets lock = 1 in ONE atomic step.
shared lock = 0

thread T(i) for i in 0..1:
    local temp = 0
    loop:
        noncritical
        while true:
            atomic:
                temp = lock
                lock = 1
            if temp == 0:
                break
        critical
        lock = 0
