# 09 · Recall sheet

Cover the right-hand column. Say each answer out loud, or write it on a
blank page. If you hesitate, re-read that part of the [README](README.md).

| Question | What you should be able to say or write |
|---|---|
| What are the readers–writers rules? | Readers may read together; a writer must be alone (no reader, no other writer). |
| Write the slide 31 reader. | down(mutexr); nr++; if nr == 1 then down(rw); up(mutexr); read; down(mutexr); nr--; if nr == 0 then up(rw); up(mutexr). Writer: down(rw); write; up(rw). |
| What is wrong with slide 31? | Writers can starve: if new readers keep arriving before the last one leaves, rw is never released. |
| How does a turnstile fix it? | Readers pass down(t); up(t) on the way in. A writer does down(t) before down(rw) and holds it while waiting, so later readers queue behind it. |
| Why does the obvious dining philosophers solution deadlock? | Everyone takes the left fork; then each waits for the right fork, which the neighbour holds: a circle of waiting. |
| What is the asymmetric solution and why does it work? | Philosopher 4 takes fork 0 first (everyone takes the lower-numbered fork first). A circle would need fork numbers to increase all the way round, which is impossible. |
| What is the waiter solution? | A semaphore that starts at 4 around picking up forks: at most 4 try at once, so at least one can get both forks. |
| Sleeping barber: semaphores in slide 33? | mutex = 1 (protects waiting), customers = 0 (barber sleeps on it), barber = 0 (barber calls the next customer); plus the counter waiting. |
| What is missing in slide 33? | The haircut and "wait until it is finished"; `cutting` is never used; customers loop instead of coming back later. |
| Why must the barber wait for the customer to leave before calling the next one (with a shared cut_done)? | Otherwise the next customer can take the previous customer's cut_done token and leave before its own haircut. A semaphore token goes to whoever waits first. |
| How do you support K barbers? | A queue of customer ids under the mutex, and one semaphore per customer that its barber posts when the haircut is done. |
| Baboon crossing: which patterns? | A lightswitch per direction on rope_free (one direction at a time), a multiplex of 5 (capacity), a turnstile (no starvation). |
| Single-lane bridge, Lab 04 Ex 1: can a direction starve? | Yes, with only lightswitches + capacity. Add a turnstile to fix it (Ex 2). |
| River crossing: valid groups? | 4 hackers, 4 serfs, or 2 + 2. |
| River crossing: how do you stop a 5th thread boarding? | The captain (the thread that completes a group) keeps the mutex until it has rowed, posts one queue token per seat, and waits until the other 3 have boarded. |
| When can everyone cross? | When H and S are both even and H + S is a multiple of 4. Every group takes an even number of each kind. |
| How do you justify a design? | One sentence per safety rule (which semaphore, which invariant), one for no deadlock (who wakes every waiter), one for no starvation (can newcomers overtake for ever?). |

**From memory:** write the turnstile readers–writers solution (exercise 1);
write the baboon crossing solution and justify its three rules; draw the
5-philosopher deadlock as a table.
