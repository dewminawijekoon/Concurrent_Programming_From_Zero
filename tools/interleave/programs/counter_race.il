# L01/L03: counter++ is really load, add, store. Two threads, one increment each.
shared counter = 0

thread T(i) for i in 0..1:
    local tmp = 0
    tmp = counter           # load
    tmp = tmp + 1           # add (local, invisible to others)
    counter = tmp           # store
    print counter
