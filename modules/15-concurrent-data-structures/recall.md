# 15 · Recall sheet

Cover the right-hand column. Say each answer out loud, or write it on a
blank page. If you hesitate, re-read that part of the [README](README.md).

| Question | What you should be able to say or write |
|---|---|
| Why are locks slow for shared data structures? (L12 slide 5) | The code under the lock runs one thread at a time (Amdahl); every core fights over the lock variable; a paused lock holder delays everyone. |
| Coarse- vs fine-grained locking? | Coarse: one lock for the whole structure. Fine: a lock per part (node), so threads in different parts can work at the same time. |
| How does hand-over-hand locking walk a list? | Hold the locks of prev and curr; lock the next node before unlocking prev; change a link only while holding both of its nodes. |
| Why is fine-grained often slower in Lab 05's benchmark? | A lock and unlock per node visited, and threads cannot overtake each other; the operations are short, so the extra locking dominates. |
| What does CAS(x, e, n) do? | In one atomic step: if x equals e, set x to n and return true; otherwise change nothing and return false. |
| Write the CAS-loop counter. | `old = LOAD(c); while (!CAS(c, old, old+1)) old = LOAD(c); return old;` |
| Why is the CAS loop lock-free but not wait-free? | A CAS fails only because another thread's CAS succeeded, so someone always progresses; but one thread can lose every round, so its steps are unbounded. |
| Why is fetch-and-add faster than a CAS loop under contention? | One instruction that always succeeds; a CAS loop wastes failed attempts that each move the cache line. |
| Define wait-free, lock-free, obstruction-free, blocking. | Wait-free: every call finishes in a bounded number of its own steps. Lock-free: some call always finishes. Obstruction-free: a call finishes if it runs alone long enough. Blocking: a paused thread can stop others. |
| Classify: mutex, CAS loop, fetch-and-add, "one CAS then give up", spin lock. | Blocking; lock-free; wait-free; obstruction-free (as an increment); blocking. |
| Which condition for hard real-time code? | Wait-free: the only one that bounds the time of every call. |
| What is a combining tree? | Threads climb a tree, combining increments in pairs; one winner adds the total at the root and hands results back down. O(log p) per increment; bad with low contention, fixed p, O(p) memory. |
| Two-lock bounded queue: what is shared? | enqLock and the tail for enqueuers; deqLock and the head for dequeuers; one atomic `permits` counter (free places) shared by both. |
| Why a sentinel node? | The list is never really empty; a dequeue just makes the first real node the new sentinel, so it never touches the pointers that enqueuers change. |
| Why take the other side's lock before broadcasting? | So the broadcast cannot fall between a sleeper's "is it empty?" check and its wait; otherwise the wake-up is lost. |
| Why broadcast, not signal? | With "wake only on empty → non-empty", one signal can wake one consumer while a second item arrives without a signal; the other consumer sleeps forever (lost wake-up). |
| What is the split counter? | enqSidePermits (enqueuers decrement) and deqSidePermits (dequeuers increment); an enqueuer that runs out takes deqLock and moves the permits over. Fewer shared updates, but now two locks at once: fix the lock order. |
| Michael–Scott enqueue: the two steps? | Logical: CAS(last->next, NULL, node), now the node is in the queue. Physical: CAS(tail, last, node), which may fail harmlessly. |
| What is a lagging tail, and what do you do? | tail points to the second-to-last node (tail->next is not NULL). Help: CAS(tail, last, last->next), then retry. |
| Michael–Scott dequeue in one breath? | If head == tail: empty if next is NULL, else help move tail and retry. Otherwise read next->value, then CAS(head, first, next). |
| Why are dequeued nodes not freed in the exercises? | Another thread may still be about to read that node; freeing it (and reusing the address) causes use-after-free and the ABA problem. |
| What is the ABA problem? [EXTRA] | A CAS sees the same pointer value again after it changed and changed back, and wrongly succeeds. |
| What is linearizability? [EXTRA] | Each operation seems to happen at one instant between its call and return, and the results match some one-at-a-time order of those instants. |

**From memory:** write the CAS-loop counter, the Michael–Scott `enq()`
(exercise 3), and the two-lock queue's `enq()`.
