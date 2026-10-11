# Quiz puzzle (module 07): readers and a writer with a lightswitch.
# R1 prints a when it starts reading and A when it stops; R2 prints b / B;
# the writer W prints w / W.
shared readers = 0
sem mutex = 1, room = 1

proc reader_enter():
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

thread R2:
    reader_enter()
    print b
    print B
    reader_exit()

thread W:
    room.down()
    print w
    print W
    room.up()
