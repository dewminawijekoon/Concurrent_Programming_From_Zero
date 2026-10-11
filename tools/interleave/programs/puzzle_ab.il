# Exam-style puzzle (our own): which outputs are possible?
# x++ and x-- are atomic here.
shared x = 0, y = 0

thread A:
    x = 5
    while x != 5:
        y++
        x++
        print A

thread B:
    y = 5
    while y == 5:
        print B
        x--
        y--
