# Baboon crossing (L08 slide 35), our solution, small enough to check every
# order of steps: 3 baboons go east, 2 go west, the rope holds at most 2
# (the real rope holds 5; the idea is the same). Each baboon crosses once.
# The asserts check the rules: never both directions on the rope, never
# more than 2 on it. Run:
#   python3 tools/interleave/interleave.py modules/09-classical-problems/demo/baboons.il
const CAP = 2
shared east = 0, west = 0          # baboons going each way that have passed the turnstile
shared on_e = 0, on_w = 0          # baboons actually on the rope (only for the asserts)
sem turnstile = 1, rope_free = 1, rope = 2, mutex_e = 1, mutex_w = 1

thread E(i) for i in 0..2:
    for k in 1..1:
        turnstile.down()
        mutex_e.down()
        east = east + 1
        if east == 1:
            rope_free.down()
        mutex_e.up()
        turnstile.up()
        rope.down()
        on_e = on_e + 1
        assert on_w == 0 and on_e <= CAP
        on_e = on_e - 1
        rope.up()
        mutex_e.down()
        east = east - 1
        if east == 0:
            rope_free.up()
        mutex_e.up()

thread W(i) for i in 0..1:
    for k in 1..1:
        turnstile.down()
        mutex_w.down()
        west = west + 1
        if west == 1:
            rope_free.down()
        mutex_w.up()
        turnstile.up()
        rope.down()
        on_w = on_w + 1
        assert on_e == 0 and on_w <= CAP
        on_w = on_w - 1
        rope.up()
        mutex_w.down()
        west = west - 1
        if west == 0:
            rope_free.up()
        mutex_w.up()
