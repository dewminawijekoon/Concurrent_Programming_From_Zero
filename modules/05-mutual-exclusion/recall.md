# 05 · Recall sheet

Cover the right-hand column. Say each answer out loud, or write it on a
blank page. If you hesitate, re-read that part of the [README](README.md).

| Question | What you should be able to say or write |
|---|---|
| What four parts does every thread repeat? | non-critical section → entry code → critical section → exit code |
| What are the four rules for a correct solution? | (1) never two threads inside together; (2) no deadlock: if threads are trying, one gets in; (3) no starvation: every thread that tries gets in eventually; (4) a thread alone gets in straight away |
| Safety vs liveness? | Safety = "something bad never happens"; disprove it with one bad order of steps. Liveness = "something good eventually happens"; disprove it with a run that goes on forever without the good thing. |
| Where may a thread stop for good? | only in its non-critical section |
| Attempt 1 (`turn`): what goes wrong? | the threads must take turns; if one stops, the other gets stuck |
| Attempt 2 (check, then raise flag)? | both can check before either raises its flag, so both go in |
| Attempt 3 (raise flag, then check)? | both raise their flags, then each waits for the other forever: deadlock |
| Attempt 4 (raise, check, step back)? | in perfect step they keep stepping back forever (livelock); one thread can also be left out forever (starvation) |
| How does Dekker fix Attempt 4? | `turn` decides that only one thread steps back |
| LockOne vs LockTwo? | LockOne (flag only) gets stuck when both try at the same time. LockTwo (victim only) gets stuck when a thread tries alone. |
| Write Peterson's lock | `lock(i): flag[i]=true; victim=i; while (flag[j] && victim==i) {}` and `unlock(i): flag[i]=false` |
| Why can't both be inside (Peterson)? | the thread that wrote `victim` last sees the other's flag raised, so it waits |
| Why can't a thread starve (Peterson)? | if the other thread comes back in, it sets `victim` to itself, which lets the waiting thread go |
| Deadlock vs livelock? | deadlock: nobody can move. Livelock: everybody keeps moving, but nobody gets in. |
| Why do real C programs need atomic variables here? | the CPU can make a write visible to other cores late, after a later read; the compiler can keep a variable in a register |

**From memory:** write Peterson in C (exercise 3). Write Attempts 2–4 in
pseudocode, each with the order of steps that breaks it.
