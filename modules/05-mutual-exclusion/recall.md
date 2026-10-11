# 05 · Recall sheet

Cover the right-hand column and say each answer out loud, or write it on a
blank page. If you hesitate, re-read that part of the README.

| Prompt | You should be able to produce |
|---|---|
| The loop every thread runs | non-critical → pre-protocol → critical → post-protocol |
| The 4 requirements (L05) | mutual exclusion · no deadlock · no starvation · success without contention |
| Safety vs liveness | "nothing bad ever" (a bad *state*) vs "something good eventually" (a bad *infinite run*) |
| Where may a thread halt? | only in its non-critical section |
| Attempt 1 | one `turn`; strict alternation; fails without contention |
| Attempt 2 | check, then set; **mutual exclusion fails** (6-step schedule) |
| Attempt 3 | set, then check; **deadlock** (4-step schedule) |
| Attempt 4 | set, check, back off; **livelock + starvation** |
| Dekker | Attempt 4 + `turn` deciding who backs off |
| LockOne / LockTwo | flag only: deadlocks concurrently · victim only: deadlocks alone |
| Peterson (write it!) | `flag[i]=true; victim=i; while(flag[j] && victim==i){}` · unlock `flag[i]=false` |
| Peterson's ME proof | whoever wrote `victim` last sees the other's flag up and waits |
| Peterson's starvation-freedom | a re-entering j sets `victim=j`, releasing i |
| Deadlock vs livelock | nobody can move vs everybody moves, nobody progresses |
| Why real C needs atomics | store buffers reorder store→load; compilers cache variables |

**From memory:** write Peterson in C (ex3) and Attempts 2–4 in pseudocode,
each with its bad schedule.
