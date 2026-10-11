# 08 · Recall sheet

Cover the right-hand column. Say each answer out loud, or write it on a
blank page. If you hesitate, re-read that part of the [README](README.md).

| Question | What you should be able to say or write |
|---|---|
| What are the two problems with busy-waiting (L09 slide 3)? | It wastes CPU time. And on one core with priorities, a high-priority thread spinning for a low-priority thread inside the critical section stops that thread from ever running, so both are stuck. |
| What is the fix for busy-waiting? | Sleep and wakeup: the waiting thread sleeps (uses no CPU) and the thread that makes the change wakes it. |
| Why are semaphores called error-prone? | It is easy to forget or repeat an `up`/`down`, use the wrong semaphore, or miss a critical section. Semaphores are global, so you must read all the code to check them. The same tool is used for mutual exclusion and for waiting, so the code does not show which job each call does. |
| What is a monitor? | An object that keeps shared data private and offers operations on it. Only one thread at a time runs inside (mutual exclusion for free). Condition variables let threads inside it sleep until a condition holds. |
| What do `wait(c)` and `signal(c)` do? | `wait` puts the thread to sleep on `c` and gives up the monitor. `signal` wakes one thread sleeping on `c`; if there is none, nothing happens and nothing is remembered. |
| How does a signal differ from a semaphore `up()`? | `up()` adds 1 to a counter that stays there. A signal with no waiter is lost. |
| Name the three signalling disciplines and who runs after B signals A (C waits at the door). | Signal and exit: B leaves at once, A runs, then C. Signal and wait: A runs at once, then B finishes, then C. Signal and continue: B finishes first, then A and C compete, and C may go first. |
| Which discipline do Java and Pthreads use? | Signal and continue, and other threads may barge in. |
| Why must `wait` be inside a `while` loop? | Under signal and continue the condition may be false again when the woken thread runs: the signaller may have changed it, another thread may have barged in, or the wake-up was spurious. The `while` checks again and sleeps again if needed. |
| Tell the C1/C2/P1 story (L09 slide 24). | C1 sleeps on an empty buffer. P1 adds an item and signals C1. Before C1 gets back in, C2 enters and takes the item. C1 then runs and, with `if`, takes from an empty buffer. |
| What does `pthread_cond_wait(&c, &m)` do? | You must hold `m`. In one atomic step it unlocks `m` and sleeps on `c`. When woken it locks `m` again before returning. |
| Signal or broadcast? | Signal wakes at least one waiter: use it when one waiter is enough and any waiter can use the change. Broadcast wakes all: use it when several may go (a writer leaves, the barrier opens, the last sentinel is taken) or when unsure. With `while` loops, broadcast is never wrong, only slower. |
| What is a lost wake-up? Give two causes. | A thread starts sleeping after its signal was already sent, so it sleeps for ever. Causes: no state variable (waiting without checking), or changing the state and signalling without holding the mutex, so the change slips in between the check and the wait. Also: signalling too rarely (L12: signal only when the queue becomes non-empty). |
| Why does holding the mutex prevent a lost wake-up? | The waiter holds the mutex from its check until `pthread_cond_wait` releases it while starting to sleep. A signaller that needs the mutex can only change the state before the check (seen) or after the waiter is asleep (heard). |
| Why two condition variables (`not_full`, `not_empty`) in the bounded buffer? | So a signal reaches the right kind of thread. With one, a producer's signal can wake another producer, which goes back to sleep, and the consumer that should have woken never does: everyone can end up asleep. |
| Monitors vs semaphores: which is more powerful? Which is safer? | Same power: each can be built from the other. Monitors are safer: mutual exclusion is automatic, data is private, and waiting is separated from mutual exclusion. |
| Readers-writers: the rules? | Many readers together, or one writer alone. Never a writer with anyone else. |
| Readers' preference vs writer priority? | Readers' preference: new readers enter while a writer only waits, so writers can starve. Writer priority: once a writer waits (`waiting_writers > 0`), new readers wait, so readers can starve. |
| [EXTRA] Why does a reusable barrier need a generation counter? | The last thread resets `arrived` to 0. Waiters that re-check `arrived < n` see 0 and sleep for ever. Waiting while the round number is unchanged avoids this. |

**From memory:** write the Pthreads bounded buffer (`put`/`get` with
`not_full`/`not_empty`), the counting semaphore from a mutex and a
condition variable (exercise 3), and the four reader-writer functions with
writer priority. For each `wait`, say why it is in a `while` loop.
