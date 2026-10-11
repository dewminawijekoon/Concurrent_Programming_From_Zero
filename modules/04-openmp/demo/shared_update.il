# demo/shared_update.il - quiz Q10(c): x = 10 is shared, threads 0, 1, 2 each do x += id
# with no critical/atomic. 'done' stands for the barrier at the end of the region.
#   python3 tools/interleave/interleave.py modules/04-openmp/demo/shared_update.il --outputs
#   python3 tools/interleave/interleave.py modules/04-openmp/demo/shared_update.il --why 10
shared x = 10, done = 0
thread T(i) for i in 0..2:
    local tmp = 0
    tmp = x
    x = tmp + i
    done = done + 1
thread Main:
    await done == 3
    print x
