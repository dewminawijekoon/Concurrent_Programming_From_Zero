# 07 · Recall sheet

Cover the right-hand column. Say each answer out loud, or write it on a
blank page. If you hesitate, re-read that part of the [README](README.md).

| Question | What you should be able to say or write |
|---|---|
| What is a semaphore? | A shared counter that never goes below 0, with two atomic operations: `down` (take one, or sleep if it is 0) and `up` (wake a sleeper, or add one). |
| What are the other names for down and up? | down = P = wait = `sem_wait`; up = V = signal = `sem_post`. |
| General vs binary semaphore? | A general (counting) semaphore can hold any value 0, 1, 2, …; a binary one only 0 or 1. |
| How is a semaphore different from a mutex? | Nobody owns a semaphore: any thread may `up` it, even one that never did `down`. That is why it can be used for signalling. |
| What are the two invariants of a semaphore? | S ≥ 0, and S = S0 + #up − #down (counting only finished downs). |
| Prove mutual exclusion for "down(S); CS; up(S)" with S = 1. | #CS = #down − #up and S = 1 + #up − #down, so #CS + S = 1. Since S ≥ 0, #CS ≤ 1. |
| Why can it not deadlock? | If everyone slept, S = 0 and #CS = 0, so #CS + S = 0, which breaks #CS + S = 1. |
| Which semaphore kinds can starve a thread? | Busy-wait: yes, even with 2 threads. Blocked-set: yes with 3 or more. Blocked-queue (first come, first served): no. |
| Signalling pattern? | Semaphore starts at 0. Thread 1: A; up. Thread 2: down; B. So A happens before B. |
| Rendezvous pattern (Bill and Ben)? | Two semaphores at 0. Each thread: up(mine); down(theirs). Never let both threads do down first: deadlock. |
| Multiplex? | A semaphore that starts at N: down; at most N threads here; up. |
| Lightswitch? | A counter with its own mutex. The first thread in does down(room); the last one out does up(room). Readers use it; writers use the room directly. |
| What is the weakness of the lightswitch readers-writers solution? | Readers are preferred: while readers keep arriving, a writer can wait forever (starvation). |
| Turnstile? | down(t); up(t). Threads pass one at a time; whoever holds t at 0 stops everyone. |
| Infinite buffer: which semaphore, and why no mutex? | `elements` = 0, counts items. One producer writes only tail, one consumer writes only head, so they never write the same variable. |
| Circular buffer semaphores? | spaces = N (free slots), elements = 0 (filled slots), plus mutex = 1 if there are several producers or consumers. |
| Why the mutex with several producers? | Two producers can both write buffer[tail] before either moves tail: one item is lost. |
| Where does the mutex go? | Inside: down(spaces) then down(mutex). Taking the mutex first and then sleeping on a full buffer is a deadlock. |
| Why does a counter + one gate barrier break when reused? | A fast thread passes, comes back to the barrier, and takes a gate token meant for a slow thread of the previous round, so it passes the next barrier alone. |
| How does the two-phase barrier fix it? | Phase 1 counts in and opens gate 1 n times; phase 2 counts out and opens gate 2 n times. Nobody can return to gate 1 until all n have passed it. |
| POSIX calls? | `sem_init(&s, 0, value)`, `sem_wait`, `sem_post`, `sem_trywait` (never waits; returns −1 if the value is 0), `sem_destroy`. Not on macOS: use the dev container. |

**From memory:** write the bounded buffer with three semaphores (exercise
5). Write the reusable two-phase barrier. Draw the value trace of `S` for
three threads using `down(S); CS; up(S)`.
