# Model exercise 1 - "while, not if" (L09 slides 23-25: consumers C1, C2, producer P1)
#
# The explorer has no condition variables, so we build a tiny monitor by hand:
#   m.down() / m.up()     enter / leave the monitor (its mutex)
#   wait(notEmpty)        is modelled as three steps:
#                            m.up()            let go of the monitor
#                            await count > 0   sleep until an item is there
#                            m.down()          get back into the monitor
# The gap between "await" and "m.down()" is the important part: another
# consumer can barge into the monitor there (signal-and-continue, as in Java
# and Pthreads).
#
# 1. Run it as it is and read the order of steps the explorer prints:
#      python3 tools/interleave/interleave.py modules/08-monitors-and-condition-variables/exercises/models/m1_while_not_if.il
#    Match each step to the story on L09 slide 24. Who barges in?
# 2. Fix the consumer so that the assertion can never fail. Change as little
#    as possible (one word is enough). Run the explorer again.
# 3. Explain in two sentences why your fix works.
#
# Test:  make test M=08 T=m1_while_not_if

shared count = 0       # items in the buffer
mutex m                # the monitor's lock

thread P:              # the producer puts two items
    m.down()
    count = count + 1
    m.up()
    m.down()
    count = count + 1
    m.up()

thread C(i) for i in 0..1:   # two consumers take one item each
    m.down()
    if count == 0:           # TODO: is checking once enough?
        m.up()
        await count > 0
        m.down()
    assert count > 0         # "I never take from an empty buffer"
    count = count - 1
    m.up()
