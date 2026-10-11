# demo/loop_index.il - the &i bug (README 3.2, quiz Q8). Main passes the address of i
# to 3 threads; thread k starts some time after it is created and prints *arg.
#   python3 tools/interleave/interleave.py modules/03-pthreads/demo/loop_index.il --outputs
#   python3 tools/interleave/interleave.py modules/03-pthreads/demo/loop_index.il --why "0 0 2"
shared i = 0, started[3] = false
thread Main:
    i = 0
    started[0] = true
    i = 1
    started[1] = true
    i = 2
    started[2] = true
    i = 3
thread T(k) for k in 0..2:
    await started[k]
    print i
