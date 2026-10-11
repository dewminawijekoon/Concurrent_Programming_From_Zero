# Quiz puzzle (module 09): readers-writers WITH a turnstile.
# R1 prints a / A when it starts / stops reading, R2 prints b / B.
# R2 prints y when it arrives. The writer prints x once it has closed the
# turnstile (so x means "a writer is now waiting"), then w / W.
shared readers = 0
sem mutex = 1, room = 1, turnstile = 1

proc reader_enter():
    turnstile.down()
    turnstile.up()
    mutex.down()
    readers = readers + 1
    if readers == 1:
        room.down()
    mutex.up()

proc reader_exit():
    mutex.down()
    readers = readers - 1
    if readers == 0:
        room.up()
    mutex.up()

thread R1:
    reader_enter()
    print a
    print A
    reader_exit()

thread W:
    turnstile.down()
    print x
    room.down()
    print w
    print W
    turnstile.up()
    room.up()

thread R2:
    print y
    reader_enter()
    print b
    print B
    reader_exit()
