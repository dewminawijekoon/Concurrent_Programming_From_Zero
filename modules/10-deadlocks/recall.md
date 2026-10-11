# 10 · Recall sheet

Cover the right-hand column. Say each answer out loud, or write it on a
blank page. If you hesitate, re-read that part of the [README](README.md).

| Question | What you should be able to say or write |
|---|---|
| Define deadlock | A set of processes is deadlocked if each one waits for an event that only another process in the set can cause (usually releasing a resource). |
| Preemptable vs non-preemptable? | Preemptable: can be taken away and given back without harm (memory). Non-preemptable: taking it away makes the owner fail (a printer mid-job, a lock mid-update). |
| The four conditions | Mutual exclusion, hold and wait, no preemption, circular wait. All four are needed; remove one and deadlock is impossible. |
| Resource-allocation graph arrows | resource → process = "held by"; process → resource = "waits for". One instance per resource: cycle ⇔ deadlock. |
| The four strategies | Ignore (ostrich), detect and recover, avoid (Banker's), prevent (attack a condition). |
| E, A, C, R | E = existing totals; A = free now; C = what each holds; R = what each is waiting for. Held + free = existing. |
| Detection algorithm | Find an unmarked process whose R row ≤ A; add its C row to A; mark it; repeat. Unmarked at the end = deadlocked. |
| Three ways to recover | Preemption (take a resource away), rollback (restart from a checkpoint), killing a process (its resources are freed). |
| Safe state? | There is some order in which every process can get its full maximum and finish. Unsafe means deadlock is possible, not certain. |
| Need matrix | Need = Max − Allocation |
| Banker's safety algorithm | work = Available; repeatedly pick an unfinished process with Need ≤ work; work += its Allocation; mark it; safe if all get marked. |
| Banker's request check | Request ≤ Need (else error), request ≤ Available (else wait), pretend to grant, run the safety check: safe → grant, unsafe → undo and wait. |
| Why is avoidance rarely used? | Maximum needs are rarely known in advance, and processes and resources come and go. |
| Attack mutual exclusion | Spooling: only a daemon uses the printer, others queue jobs. Not possible for every resource. |
| Attack hold and wait | Request everything at the start (or release all before asking for more). Needs are often unknown; resources sit idle. |
| Attack no preemption | Take resources away by force: usually not viable (half-printed page). |
| Attack circular wait | Number the resources; request only in increasing order. A cycle would need the numbers to go up all the way round and return to the start: impossible. |
| Fix two-lock and bank-transfer deadlocks | Lock in one global order (smaller id first); for transfer(a, a) take the lock once. |

**From memory:** write `is_safe()` (exercise 4). Run the Banker's
algorithm by hand on the table in section 7 of the README, then check it
with the calculator.
