# Quiz puzzle: two threads each raise their own flag, then look at the other's.
shared a = 0, b = 0
thread P:
    a = 1
    if b == 0:
        print P
thread Q:
    b = 1
    if a == 0:
        print Q
