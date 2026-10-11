# 07 · Quiz: semaphores and synchronization patterns

Answer on paper first, as you would in the exam, then check
[`answers/07-semaphores/quiz-answers.md`](../../answers/07-semaphores/quiz-answers.md).
Marks in brackets show how much detail is expected. Tags: `[L08]` lecture,
`[Lab04]` lab, `[tf]` true/false + justification, `[trace]` give a
schedule, `[output]` which outputs are possible, `[proof]`, `[calc]`,
`[code]`, `[design]`.

The puzzle programs for section C are in [`demo/`](demo). Run them with
the explorer only **after** you have written your answer.

## A. True or false? Justify briefly. (2 marks each)

1. `[tf][L08]` If a binary semaphore has the value 1 and a thread does `up` on it, its value becomes 2.
2. `[tf][L08]` With the blocked-set semaphore of slide 9, `up(S)` always adds 1 to S.
3. `[tf][L08]` The one-semaphore mutual exclusion solution (slide 6) is starvation-free for any number of processes, whatever kind of semaphore is used.
4. `[tf][Lab04]` A semaphore that starts at 0 can make one thread wait until another thread has done something.
5. `[tf][L08]` In the circular buffer of slide 25, the semaphore `spaces` stops two producers from writing into the same slot.
6. `[tf][L09][Lab04]` A barrier made of one counter, one mutex and one gate semaphore (the last thread posts the gate n times and resets the counter) can safely be used inside a loop.
7. `[tf][L08]` In the readers-writers solution of slide 31, a writer can wait forever even though every reader finishes reading.

## B. Short answer

8. `[calc][L08]` (3 marks) A semaphore S starts at 2. So far, 6 `up` operations and 7 `down` operations have finished. (a) What is S? (b) Could 9 `down` operations have finished, with still only 6 `up`s? Use the invariants of slide 5.
9. `[proof][L08]` (4 marks) For the slide 6 solution, prove that `#CS + S = 1` is an invariant, and use it to prove mutual exclusion.
10. `[proof][L08]` (3 marks) Using the same invariant, prove that the slide 6 solution cannot deadlock.
11. `[trace][L08]` (3 marks) With a **busy-wait** semaphore, give an order of steps in which P2 starves while P1 enters again and again. Number the steps.
12. `[trace][L08]` (4 marks) With a **blocked-set** semaphore and three processes, describe a run in which one process starves. Why is this impossible with a blocked-queue semaphore?
13. `[L08]` (2 marks) Slide 21 (infinite buffer, one producer, one consumer) asks: "Mutual exclusion is not an issue. Why?" Answer it.
14. `[trace][L08]` (4 marks) Two producers use the slide 25 circular buffer (no mutex). Give an order of steps in which an item is lost.
15. `[design][L08]` (3 marks) A student adds a mutex to the circular buffer like this:
    ```c
    producer:  down(mutex); down(spaces); store; move tail; up(elements); up(mutex);
    consumer:  down(mutex); down(elements); take; move head; up(spaces); up(mutex);
    ```
    What can go wrong? Give the situation and fix the code.

## C. Is this output possible? (exam style)

16. `[output]` (4 marks) `demo/q_three_threads.il`:
    ```
    sem s1 = 0, s2 = 1
    X:  down(s2); print A; up(s1)
    Y:  down(s1); print B; up(s2)
    Z:  down(s2); print C; up(s1)
    ```
    Which of these outputs are possible: `A B C`, `A C B`, `B A C`, `C B A`? How many different outputs can the program print in total? Justify.

17. `[output]` (6 marks) `demo/q_lightswitch_order.il`: two readers and one writer use the lightswitch solution (slide 31). Reader R1 prints `a` when it starts reading and `A` when it stops; reader R2 prints `b` / `B`; the writer prints `w` / `W`. For each output, say whether it is possible, and why:
    (a) `a b B A w W`  (b) `a w A W b B`  (c) `w W b a A B`  (d) `b a A w B W`

18. `[output]` (5 marks) `demo/q_lightswitch_bug.il` is the same program, except that a reader tests `if (readers == 1)` **after** releasing the counter's mutex. Show that the output `a w A W b B` is now possible: give the order of steps. What is the value of `room` at the end of your run, and why is that a second bug?

19. `[output]` (3 marks) `demo/q_buffer_two_slots.il`: one producer, one consumer, a buffer with 2 slots, 3 items. The producer prints `p` after storing an item; the consumer prints `c` after taking one. Is `p p p c c c` possible? List every possible output.

20. `[calc][L08]` (4 marks) Timing with the lightswitch solution (slide 31). Reader R1 arrives at time 0 and reads for 10 s. A writer W arrives at 2 s and writes for 3 s. Reader R2 arrives at 4 s and reads for 2 s. Reader R3 arrives at 9 s and reads for 4 s. Give the start and finish time of each thread. What does this show about the solution?

## D. Lab-style and code

21. `[design][Lab04]` (4 marks) Lab 04 Q5.1: why can't a simple counter plus a single semaphore be used as a **reusable** barrier? Describe the failure step by step, then explain how the two-phase barrier prevents it.
22. `[Lab04]` (3 marks) Lab 04 Q5.2: in the parallel iterative averaging, (a) why do you need **two** barrier calls per step, and (b) should the parallel result be exactly the same as the serial one, or only close? Explain.
23. `[code][L08]` (5 marks) From memory: write the producer and the consumer for a circular buffer of N slots shared by **several** producers and **several** consumers. Give each semaphore's starting value and say what it counts.
24. `[code][L08]` (4 marks) Bill and Ben (slide 14): give the semaphore declarations and the code for SYNCH 1 and SYNCH 2. Then show that if **both** SYNCHs do their `down` before their `up`, the program deadlocks.
25. `[design][L09]` (3 marks) A web server must never handle more than 20 requests at the same time. Which pattern do you use? Write the code each request runs, and explain with an invariant why there are never more than 20.
