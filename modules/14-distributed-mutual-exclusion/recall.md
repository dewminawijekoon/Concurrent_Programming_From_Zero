# 14 · Recall sheet [EXTRA]

Cover the right-hand column. Say each answer out loud, or write it on a
blank page. If you hesitate, re-read that part of the [README](README.md).

| Question | What you should be able to say or write |
|---|---|
| Why don't shared-memory locks work here? | There is no shared variable to read or test-and-set; processes only exchange messages, which are slow and may arrive in any order. |
| The three requirements? | Safety: at most one process inside. Liveness: every request is eventually granted (no deadlock, no starvation). Ordering: a request that happened before another is granted first. |
| How do we measure the algorithms? | Messages per entry, client delay (waiting time without competition), synchronization delay (gap between one leaving and the next entering). |
| What is happened-before? | a → b if a comes before b in one process, or a is a send and b its receipt, or a chain of these links them. Otherwise the events are concurrent. |
| Lamport clock rules? | Local event: +1. Send: +1 and stamp the message. Receive stamped t: clock = max(clock, t) + 1. |
| What do Lamport timestamps guarantee? | If a → b then C(a) < C(b). Not the reverse. Ties broken by process id give a total order. |
| Centralized coordinator? | Request → grant → release via one coordinator with a queue: 3 messages per entry; single point of failure and bottleneck. |
| Token ring? | One token goes round a logical ring; only the holder may enter. Safe because there is one token; waits at most n − 1 passes; costs 1 to unbounded messages per entry; lost token is the problem. |
| Ricart–Agrawala entry and exit? | Entry: WANTED, timestamp T, REQUEST to all n − 1, enter after n − 1 replies. Exit: RELEASED, reply to all deferred. |
| Ricart–Agrawala: reply or defer? | Defer if HELD, or if WANTED and my (T, id) is earlier than the requester's; otherwise reply at once. |
| Cost of Ricart–Agrawala? | 2(n − 1) messages per entry; client delay 2 message times; synchronization delay 1. Any crash blocks everybody. |
| Why is Ricart–Agrawala safe (one line)? | Of two competing requests, the process with the earlier one defers the other until it leaves; a process that had not requested yet would get a later timestamp. |
| Maekawa? | Ask only a quorum; every two quorums overlap and each member grants one at a time. About 3√n (best) to 6√n (grid) messages; basic version can deadlock. |

**From memory:** write Ricart–Agrawala's three handlers (exercise 3), and
compute Lamport timestamps for a three-process picture.
