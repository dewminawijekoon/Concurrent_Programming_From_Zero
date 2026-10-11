# 01 · Recall sheet

Cover the right-hand column. Say each answer out loud, or write it on a
blank page. If you hesitate, re-read that part of the [README](README.md).

| Question | What you should be able to say or write |
|---|---|
| What is a race condition? | The result depends on the order in which the threads' steps happen, and some orders give a wrong result. |
| What three steps is `counter++` really? | Load the counter into a register, add 1 to the register, store the register back into the counter. |
| Draw the lost-update timeline. | T1 loads 5, T2 loads 5, T1 stores 6, T2 stores 6: two increments, but the counter only went up by 1. |
| Why is the racy result never above N × M? | Every store writes one more than a value that was already in memory, so updates can be lost but never invented. |
| What is an interleaving? | One possible order of all the threads' atomic steps, keeping each thread's own steps in order. |
| When is a concurrent program correct (L03)? | When it is correct for every fair interleaving. One bad interleaving is enough to make it wrong. |
| How many interleavings for n threads with m steps each? | (n·m)! / (m!)ⁿ. Example: 3 threads × 2 steps = 720 / 8 = 90. |
| Why divide by (m!)ⁿ? | Each thread's own m steps must stay in order, so only 1 of the m! orders of each thread's steps is allowed. |
| Possible values of x for `x = y + z` vs `y = 1; z = 2`? | 0, 1, 2 and 3 (if y is loaded before z). If z is loaded first, only 0, 1 and 3. |
| Possible values for `c1 = c1 * c2` vs `c1 = c1 + c2` (c1 = 2, c2 = 3)? | 9 or 15 if each line is atomic; also 5 and 6 on a load-store machine (lost update). |
| What is check-then-act? | A thread tests a condition, then acts on it, but another thread changes the condition in between. Check and act must be in one critical section. |
| What went wrong in L01's shared prime counter? | `getAndIncrement` was a separate read and write, so two threads could get the same number, and the counter could even go backwards. |
| Static vs dynamic load balancing? | Static: fixed blocks decided up front; unequal work leaves some threads idle. Dynamic: threads take the next piece from a shared counter, so fast threads take more. |
| Three ways to fix a race on a counter? | A mutex around the read-modify-write; an atomic instruction (`atomic_fetch_add`); or no sharing: local sums combined once at the end. |
| Why is the local-sum version (Lab 01 Ex 3 Part B) faster? | It takes the lock N times instead of N × M times, so the threads almost never wait for each other (no lock contention). |
| What happens if you forget to unlock? | The next `lock()` waits forever: the program hangs (deadlock). |
| Safety vs liveness? | Safety: something bad never happens (disprove with one finite run). Liveness: something good eventually happens (disprove with an infinite run). |
| L01 vs L03 on deadlock? | L01 calls "no deadlock" liveness; L03 lists it under safety. Most books say liveness. State your definition. |
| How do you find races? | Make bad timing likely (yield inside the window), run ThreadSanitizer (`make tsan`), and check small models with the interleaving explorer. |

**From memory:** write a thread-safe `get_and_increment` with a mutex, and
write the lost-update timeline for two `counter++`.
