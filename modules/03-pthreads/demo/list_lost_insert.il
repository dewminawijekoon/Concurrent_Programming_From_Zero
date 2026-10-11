# demo/list_lost_insert.il - two unlocked inserts into the list 2 -> 7 (quiz Q15).
# A inserts 3, B inserts 4, both right after node 2. C prints next2, next3, next4 at the end.
# Both threads have already walked the list and decided that their new node
# goes right after node 2. Each insert is: read next2, point the new node
# there, set next2 to the new node.
#   "3 4 7" = 2 -> 3 -> 4 -> 7 (fine)    "3 7 7" / "4 7 7" = a node was lost
#   "4 7 3" = 2 -> 4 -> 3 -> 7 (every node is there, but the list is no longer sorted)
#   python3 tools/interleave/interleave.py modules/03-pthreads/demo/list_lost_insert.il --why "4 7 7"
shared next2 = 7, next3 = 0, next4 = 0, doneA = false, doneB = false
thread A:
    local p = 0
    p = next2
    next3 = p
    next2 = 3
    doneA = true
thread B:
    local p = 0
    p = next2
    next4 = p
    next2 = 4
    doneB = true
thread C:
    await doneA and doneB
    print next2
    print next3
    print next4
