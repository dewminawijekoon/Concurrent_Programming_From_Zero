# Quiz puzzle: which outputs can go on forever?
shared x = 0
thread A:
    loop:
        x = 1
        print A
thread B:
    loop:
        if x == 1:
            x = 0
            print B
