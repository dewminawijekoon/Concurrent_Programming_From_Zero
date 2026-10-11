# 09 · Quiz: classical synchronization problems

Answer on paper first, as you would in the exam, then check
[`answers/09-classical-problems/quiz-answers.md`](../../answers/09-classical-problems/quiz-answers.md).
Marks in brackets show how much detail is expected. Tags: `[L08]` lecture,
`[Lab04]` lab, `[tf]` true/false + justification, `[trace]` give a
schedule, `[output]` which outputs are possible, `[proof]`, `[calc]`,
`[code]`, `[design]`.

## A. True or false? Justify briefly. (2 marks each)

1. `[tf][L08]` The readers–writers solution of slide 31 guarantees that every writer eventually writes.
2. `[tf][L08]` In the deadlock of the dining philosophers, two philosophers hold the same fork.
3. `[tf][L08]` In the asymmetric solution (slide 28), philosopher 4 picks up its left fork first.
4. `[tf][L08]` In the sleeping barber solution (slide 33), the semaphore `cutting` is needed to stop the barber cutting two customers at once.
5. `[tf][Lab04]` A single-lane bridge solution built from one lightswitch per direction plus a capacity semaphore cannot starve either direction.
6. `[tf][Lab04]` In the river crossing, with 6 hackers and 2 serfs, everybody can always cross.

## B. Short answer

7. `[trace][L08]` (3 marks) Give an order of steps in which the obvious dining philosophers solution (everyone takes fork i, then fork i+1) deadlocks with 5 philosophers.
8. `[proof][L08]` (4 marks) Explain why the asymmetric solution of slide 28 cannot deadlock.
9. `[proof][L08]` (3 marks) Prove that in the slide 31 readers–writers solution a writer and a reader are never in the database together.
10. `[trace][L08]` (4 marks) Show, with a timeline of arrivals, that a writer can starve in the slide 31 solution.
11. `[design][L08]` (4 marks) Add a turnstile to the slide 31 solution so that writers cannot starve. Write the reader and writer code, and explain why a newly arriving reader can no longer overtake a waiting writer.
12. `[design][EXTRA]` (3 marks) Describe the "waiter" solution for the dining philosophers (at most 4 may try to eat). Why can it not deadlock?
13. `[L08]` (3 marks) In the sleeping barber of slide 33, explain the job of each of `mutex`, `customers` and `barber`, and say what is wrong (or missing) in the slide's code.

## C. Is this output possible? (exam style)

14. `[output]` (4 marks) Two readers and a writer. R1 prints `a` / `A` when it starts / stops reading, R2 prints `b` / `B`. R2 prints `y` when it arrives. The writer prints `x` when it is waiting for the room, then `w` / `W` when it starts / stops writing. Is the output `a x y b A B w W` possible:
    (a) with the slide 31 solution (`demo/q_rw_plain.il`)?
    (b) with the turnstile solution (`demo/q_rw_turnstile.il`; there, `x` is printed right after the writer has closed the turnstile)?
    Explain the difference.
15. `[trace]` (4 marks) Three philosophers, the obvious solution (model m1). Give the explorer's deadlock trace in your own words and give the state of each philosopher and each fork at the end. Then explain why the asymmetric version (model m2) has no such trace.

## D. Design and justification (exam style)

16. `[design][L08]` (8 marks) The baboon crossing (slide 35): rope across a canyon; never baboons going opposite ways on it; at most 5 on it; a continuous stream one way must not block the other way for ever. Give a semaphore solution (declarations with starting values, and the code a baboon going east runs) and justify each of the three rules.
17. `[design][Lab04]` (5 marks) Lab Q1.1 and Q1.2: for the single-lane bridge (one direction at a time, at most 3 cars), how many semaphores do you need, and what does each represent? Can one direction starve? If so, how do you fix it?
18. `[design][Lab04]` (4 marks) Lab Q2.2: you measure the total time for 15 + 15 trucks with N = 1, 3 and 5. What do you expect, and why does the improvement get smaller as N grows?
19. `[design][Lab04]` (5 marks) Lab Q3.3: extend the barbershop to K barbers. What changes, and why does the one-barber "`barber_ready` + `cut_done`" design break with several barbers?
20. `[design][Lab04]` (5 marks) Lab Q4.1 and Q4.2: how does your river crossing ensure that only valid groups of 4 board, and that no 5th thread joins a boat? Can it deadlock?
21. `[calc][Lab04]` (3 marks) Lab Q4.3 and more: which of these can all cross? (a) H = 4, S = 4; (b) H = 8, S = 0; (c) H = 6, S = 2; (d) H = 5, S = 3. Give a general rule and justify it.
22. `[Lab04]` (3 marks) Lab Q3.1: how do you expect the number of turned-away customers to change for M = 1, 3, 10 waiting chairs (C = 20)? If all 20 customers arrive at the same moment, what is the smallest M that **guarantees** nobody is turned away? (Careful: think about when a customer stops counting as "waiting".)
23. `[Lab04]` (3 marks) Lab Q2.3: compare the semaphore solution of the construction zone with a mutex + condition variable solution (Lab 02). Which is easier to read? Which gives more control over fairness?
