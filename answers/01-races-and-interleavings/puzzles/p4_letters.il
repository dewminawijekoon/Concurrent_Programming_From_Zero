# Quiz puzzle: which strings of letters can this print?
shared x = 0
thread A:
    x = 1
    print A
    if x == 2:
        print A
thread B:
    if x == 1:
        x = 2
        print B
    print B
