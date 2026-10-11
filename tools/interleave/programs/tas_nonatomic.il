# The same lock if TestAndSet were NOT atomic: the read and the write are two steps.
shared lock = 0

thread T(i) for i in 0..1:
    local temp = 0
    loop:
        noncritical
        while true:
            temp = lock
            lock = 1
            if temp == 0:
                break
        critical
        lock = 0
