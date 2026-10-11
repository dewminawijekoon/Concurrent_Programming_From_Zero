# Quiz puzzle: two threads each do n++ twice, written as load and store.
# W prints n after both threads have finished.
shared n = 0, done = 0
thread T(i) for i in 0..1:
    local t = 0
    t = n
    n = t + 1
    t = n
    n = t + 1
    done++
thread W:
    await done == 2
    print n
