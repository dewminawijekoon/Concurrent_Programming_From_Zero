# Quiz puzzle (module 07): which outputs are possible?
sem s1 = 0, s2 = 1

thread X:
    s2.down()
    print A
    s1.up()

thread Y:
    s1.down()
    print B
    s2.up()

thread Z:
    s2.down()
    print C
    s1.up()
