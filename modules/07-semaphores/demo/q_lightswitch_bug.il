# Quiz puzzle (module 07): a BUGGY lightswitch. The test "am I the first?"
# was moved out of the mutex. Same prints as q_lightswitch_order.il.
shared readers = 0
sem mutex = 1, room = 1

proc reader_enter():
    mutex.down()
    readers = readers + 1
    mutex.up()
    if readers == 1:          # BUG: another reader may have changed readers by now
        room.down()

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
