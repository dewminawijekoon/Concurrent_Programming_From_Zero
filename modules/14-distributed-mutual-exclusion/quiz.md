# 14 · Quiz: distributed mutual exclusion [EXTRA]

This topic is not in the lecture slides (syllabus week 12 only), so no
question here comes from a slide. Answer on paper first, then check
[`answers/14-distributed-mutual-exclusion/quiz-answers.md`](../../answers/14-distributed-mutual-exclusion/quiz-answers.md).
Tags: `[L03]` links to the happens-before slides, `[EXTRA]` for the rest;
`[tf]`, `[calc]`, `[trace]`, `[proof]`, `[design]`.

## A. True or false? Justify briefly. (2 marks each)

1. `[tf][EXTRA]` Peterson's lock can be used between processes on different computers if they send each other their `flag` values.
2. `[tf][L03]` If Lamport timestamp C(a) < C(b), then a happened before b.
3. `[tf][L03]` If a happened before b, then C(a) < C(b).
4. `[tf][EXTRA]` In the token ring, a process that wants the critical section may have to wait for n − 1 token passes.
5. `[tf][EXTRA]` In Ricart–Agrawala, a process that is inside the critical section replies to every request at once, so others are not delayed.
6. `[tf][EXTRA]` The centralized coordinator algorithm needs fewer messages per entry than Ricart–Agrawala when n > 2.
7. `[tf][EXTRA]` Ricart–Agrawala keeps working if one process crashes, because there is no coordinator.

## B. Calculations and traces

8. `[calc][L03]` (4 marks) Three processes start with clocks at 0. P0: local event, then sends m1 to P2. P1: sends m2 to P2, then a local event. P2: local event, receives m2, receives m1, local event. Give every event's Lamport timestamp. Name one pair of concurrent events.
9. `[calc][EXTRA]` (4 marks) For n = 9 processes, how many messages does one entry cost with (a) the coordinator, (b) Ricart–Agrawala, (c) Maekawa with 3 × 3 grid quorums (row plus column)? (d) What does an entry cost in the token ring if all 9 processes always want to enter, and if only one process ever wants to?
10. `[trace][EXTRA]` (4 marks) Ricart–Agrawala with 3 processes. P0 sends REQUEST(4, P0) and P2 sends REQUEST(2, P2) at about the same time; P1 is not interested. Give, step by step, who replies at once, who defers, and the order in which P0 and P2 enter.
11. `[trace][EXTRA]` (3 marks) Ricart–Agrawala: P1 is WANTED with request (5, P1) and receives REQUEST(5, P3). What does it do, and what does P3 do with P1's request? What would go wrong without the tie-break by id?

## C. Proofs and design

12. `[proof][EXTRA]` (4 marks) Prove that the token ring satisfies mutual exclusion and starvation-freedom.
13. `[proof][EXTRA]` (5 marks) Prove that Ricart–Agrawala satisfies mutual exclusion.
14. `[design][EXTRA]` (4 marks) A process in the token ring crashes while holding the token. What happens? Describe what the others would need to do to recover, and one danger of the recovery.
15. `[design][EXTRA]` (3 marks) Why must a Ricart–Agrawala process that has finished all its own work keep running until all others are done? How does exercise 6 know when it may stop?
16. `[design][EXTRA]` (4 marks) A shared printer is used by 50 office computers, each printing a few times a day. Which algorithm do you recommend: coordinator, token ring or Ricart–Agrawala? Justify with message costs and failures.
17. `[design][EXTRA]` (3 marks) Explain how Maekawa's quorums guarantee mutual exclusion, and why the basic version can deadlock.
