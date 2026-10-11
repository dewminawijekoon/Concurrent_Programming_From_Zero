# Lightswitch pattern (Little Book of Semaphores 4.2): the first thread in
# locks `room`, the last one out unlocks it. Readers share, the writer is alone.
shared readers = 0
sem mutex = 1, room = 1

proc switch_lock(counter, s):
    mutex.down()
    counter = counter + 1
    if counter == 1:
        s.down()
    mutex.up()

proc switch_unlock(counter, s):
    mutex.down()
    counter = counter - 1
    if counter == 0:
        s.up()
    mutex.up()

thread R1:
    switch_lock(readers, room)
    print A
    print B
    switch_unlock(readers, room)

thread W:
    room.down()
    print C
    print D
    room.up()

thread R2:
    switch_lock(readers, room)
    print E
    print F
    switch_unlock(readers, room)
