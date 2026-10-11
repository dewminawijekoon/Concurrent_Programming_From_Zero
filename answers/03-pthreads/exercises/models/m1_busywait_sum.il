# ANSWER - the global sum with busy-waiting      [L04 slides 21-26]
const N = 3
shared sum = 0, turn = 0

thread T(i) for i in 0..N-1:
    local tmp = 0
    await turn == i            # busy-wait: while (turn != my_id) {}
    tmp = sum
    sum = tmp + i + 1
    print sum
    turn = (turn + 1) % N      # hand the turn to the next thread

# (a) Only "1 3 6". The turn forces the order T0, T1, T2, and only the thread
#     whose turn it is touches sum, so no update is lost.
# (b) T2 spins at `await turn == 2`, checking turn again and again. On a real
#     computer it keeps a CPU core 100% busy doing nothing useful, a core that
#     T0 or T1 could have used.
# (c) The threads must go strictly in order. If the thread whose turn it is
#     is not running (the operating system paused it to let a spinning thread
#     run), everybody waits until it gets a core again. With more threads than
#     cores this happens again and again, so the run time explodes. A mutex
#     lets waiting threads sleep and lets whoever is ready go next.
