# Quiz puzzle (module 07): bounded buffer with 2 slots, one producer and one
# consumer, 3 items. The producer prints p when it has stored an item, the
# consumer prints c when it has taken one.
sem spaces = 2, elements = 0

thread Producer:
    for k in 1..3:
        spaces.down()
        print p
        elements.up()

thread Consumer:
    for k in 1..3:
        elements.down()
        print c
        spaces.up()
