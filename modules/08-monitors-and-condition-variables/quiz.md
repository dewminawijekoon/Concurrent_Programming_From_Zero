# 08 · Quiz: monitors and condition variables

Answer on paper first, as you would in the exam, then check
[`answers/08-monitors-and-condition-variables/quiz-answers.md`](../../answers/08-monitors-and-condition-variables/quiz-answers.md).
Marks in brackets show how much detail is expected. Tags: `[L09]` lecture,
`[Lab02]` lab question, `[tf]` true/false + justification, `[trace]` give
an order of steps, `[design]`, `[calc]`, `[code]`.

## A. True or false? Justify briefly. (2 marks each)

1. `[tf][L09]` In a monitor, the programmer does not need to write any code to make two operations mutually exclusive.
2. `[tf][L09]` If `signal(c)` is called when no thread is waiting on `c`, the next thread that calls `wait(c)` returns immediately.
3. `[tf][L09]` Monitors are more powerful than semaphores: some synchronization problems can be solved with monitors but not with semaphores.
4. `[tf][L09]` Monitors are less error-prone than semaphores.
5. `[tf][L09]` Under the signal-and-exit discipline, `if (count == 0) wait(notEmpty);` is correct.
6. `[tf][L09]` Under signal and continue, a signalled thread always runs before the threads waiting on the monitor's entry queue.
7. `[tf][Lab02]` `pthread_cond_wait()` can return even though no thread called `pthread_cond_signal()` or `pthread_cond_broadcast()`.
8. `[tf][L12]` If every wait is inside a `while` loop, replacing every `signal` with `broadcast` can make the program give wrong results.
9. `[tf][L09]` A Java object has one waiting queue for each condition, like `notFull` and `notEmpty`.
10. `[tf][L09]` Because only one thread is inside a monitor at a time, programs built from monitors cannot deadlock.

## B. Lab 02 analysis questions

11. `[Lab02][design]` (3 marks) *Ping-pong (Ex 1, Q1.1).* What happens if you replace the `while` around `pthread_cond_wait()` with `if`? Your test runs show perfect output 100 times in a row. Does that prove the `if` version is correct? Explain spurious wake-ups.
12. `[Lab02]` (2 marks) *Ping-pong (Q1.2).* Does `pthread_cond_broadcast()` instead of `pthread_cond_signal()` change correctness? Why or why not?
13. `[Lab02][calc]` (3 marks) *Bounded buffer (Ex 2, Q2.1).* With `BUFFER_SIZE = 1`, is the program still correct? What does the order of "Produced"/"Consumed" lines look like, and why is it slower? (Use `make run M=08 EX=lab2_bounded_buffer ARGS="1000000 1"` and `ARGS="1000000 5"` to get your own numbers.)
14. `[Lab02][trace]` (5 marks) *Bounded buffer (Q2.2).* Why use two condition variables (`not_full`, `not_empty`) instead of one? (a) Show that with **one** producer and **one** consumer, a single condition variable with `signal` still works. (b) With buffer size 1, two producers and two consumers, give an order of steps in which a single condition variable with `signal` leaves all four threads asleep.
15. `[Lab02][design]` (3 marks) *Many producers/consumers (Ex 3, Q3.2).* The lab says `broadcast` is "generally preferred" with many producers and consumers. When is `signal` still correct, and where in lab track 3 is `broadcast` really needed?
16. `[Lab02]` (2 marks) *Q3.3.* Can items from different producers be consumed in a different order from the one in which they were produced? Is that a problem?
17. `[Lab02][design]` (3 marks) In lab track 3, consumers must stop after all M sentinels have been taken. A student writes: "each consumer keeps taking items; when it takes a `-1` it adds 1 to a shared `sentinels_seen` and stops if `sentinels_seen == M`". With N = 3 consumers this program sometimes never ends. Why? How do you fix it?
18. `[Lab02]` (2 marks) *Readers-writers (Ex 4, Q4.1).* From a timestamped log, how do you show that several readers were reading at the same time, and that no writer was writing during that time?
19. `[Lab02]` (3 marks) *Q4.2.* With R = 5 readers and W = 1 writer, the writer may wait a long time. Explain why, using the code of `start_read()`.
20. `[Lab02][design]` (4 marks) *Writer priority (Ex 5, Q5.2).* Can readers starve in the writer-priority version? Under what conditions? Sketch a design in which neither readers nor writers can starve.

## C. Traces

21. `[trace][L09]` (6 marks) Thread A is inside monitor MON and calls `wait(Q)`. Then B enters MON. Then C calls an operation of MON and must wait at the entrance. Then B calls `signal(Q)`. For each discipline (signal and exit, signal and wait, signal and continue), give the order in which A, B and C run monitor code next, and say whether A can rely on the condition it waited for.

22. `[trace][L09]` (6 marks) A monitor has `int x = 0` and a condition `Q`:
    ```
    opA():  if (x == 0) wait(Q);
            print("A", x);   x = x - 1;
    opB():  x = x + 1;   print("B", x);   signal(Q);
    ```
    Thread A calls `opA()` first and waits. Then B calls `opB()`. While B is inside, thread C calls `opA()` and waits at the entrance.
    (a) List every possible output under signal and wait.
    (b) List every possible output under signal and continue (with barging allowed). Which one is a bug, and what is the final value of `x` then?
    (c) Change one word so that (b) has no bug, and list the possible outputs again.

23. `[trace][L12]` (4 marks) To save time, a programmer changes the queue's `enq()` to signal `notEmpty` **only when the queue goes from empty to non-empty**. Show an order of steps with two consumers and two producers in which a consumer sleeps for ever although there is an item in the queue. What is this bug called?

24. `[trace]` (3 marks) The waiter does `lock(m); if (!ready) { cond_wait(c, m); } unlock(m);` (correct apart from the `if`). The signaller does `ready = 1; cond_signal(c);` **without** locking `m`. Give an order of steps in which the waiter sleeps for ever.

## D. Code

25. `[code][L09]` (5 marks) Write a counting semaphore (`init`, `down`, `up`) as a monitor in pseudocode, or in C with a mutex and a condition variable. Why is the wait inside a `while` loop? Is `signal` in `up()` enough, or do you need `broadcast`?
26. `[code][EXTRA]` (4 marks) Write `barrier_wait()` for a reusable barrier for n threads using a mutex and a condition variable. Explain why `while (arrived < n) wait;` does not work.
