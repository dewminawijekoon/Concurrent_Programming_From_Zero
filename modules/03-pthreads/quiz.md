# 03 · Quiz: Pthreads

Answer on paper first, as you would in the exam, then check
[`answers/03-pthreads/quiz-answers.md`](../../answers/03-pthreads/quiz-answers.md).
Marks in brackets show how much detail is expected. Tags: `[L04]` lecture,
`[Lab01]` lab, `[tf]` true/false + justification, `[output]` "is this output
possible?", `[trace]` give an order of steps, `[calc]`, `[design]`, `[code]`,
`[proof]`.

## A. True or false? Justify briefly. (2 marks each)

1. `[tf][L04]` A local variable declared inside the thread function is shared by all threads that run that function.
2. `[tf][L04]` The threads must be joined in the same order in which they were created; otherwise the program can hang.
3. `[tf][L04]` In the row-block matrix-vector product, the write to `y[i]` must be protected by a mutex.
4. `[tf][L04]` The busy-wait global sum gives a wrong answer when there are more threads than cores.
5. `[tf][L04]` A read-write lock lets two threads hold it for writing at the same time, as long as they change different nodes.
6. `[tf][L04]` With one mutex per node, two `Member` calls can run at the same time.
7. `[tf][Lab01]` When the Lab 01 Ex 1 program runs with N = 8, the threads always print their lines in index order 0, 1, …, 7.
8. `[tf][EXTRA]` If a multithreaded test passes 1000 times in a row, the program has no data race.

## B. Short answer

9. `[output][L04]` (4 marks) This loop starts three threads. Each thread prints `*(int *)arg` once. Main then joins them with a different loop variable.
   ```c
   for (int i = 0; i < 3; i++) pthread_create(&t[i], NULL, show, &i);
   ```
   (a) Which values can the thread created when `i` was 1 print? (b) Is the output `0 0 2` possible? Is `3 3 3`? (c) Fix the code.
   Check with `demo/loop_index.il`.
10. `[code][L04]` (3 marks) A student writes:
    ```c
    struct arg a;
    for (int t = 0; t < P; t++) { a.id = t; pthread_create(&tid[t], NULL, work, &a); }
    ```
    What is wrong, and how do you fix it?
11. `[trace][L04]` (3 marks) Two threads estimating pi both do `sum += my_term` on a shared `sum`, with no lock. Give an order of load/add/store steps that loses an update.
12. `[calc][L04]` (4 marks) A matrix has m = 10 rows and you use p = 4 threads. (a) Which rows does each thread get with `first = t*m/p`, `last = (t+1)*m/p`? (b) The lecture's rule gives thread t the rows `t*(m/p)` to `(t+1)*(m/p) - 1`. What goes wrong here?
13. `[design][L04]` (4 marks) Give three reasons why a mutex is usually better than busy-waiting for the global sum. Describe one situation where busy-waiting would be acceptable.
14. `[L04]` (3 marks) L04 slide 26 compares busy-waiting and a mutex on a machine with 2 × 4 cores, for 1 to 64 threads. Up to 8 threads both have about the same run time. Explain why the busy-wait time then grows sharply while the mutex time stays flat.
15. `[trace][L04]` (4 marks) The list is `2 -> 7`. Thread A inserts 3 and thread B inserts 4, with no locks. Both have already walked the list and found that their node goes right after node 2. Give an order of steps after which node 3 is missing from the list. Check with `demo/list_lost_insert.il`.
16. `[calc][Lab01]` (3 marks) In Lab 01 Ex 3 with N = 8 threads and M = 100,000: how many times is the mutex locked in Part A? In Part B? Explain, using the word *contention*, why Part B is so much faster.
17. `[Lab01]` (3 marks) Lab 01 Q1.2: what happens if you leave out the `pthread_join` calls in the Ex 1 program?
18. `[Lab01]` (3 marks) Lab 01 Q3.3: in Part A, a thread forgets to call `pthread_mutex_unlock`. What happens when you run the program, and why?

## C. Design and proof

19. `[design][L04]` (6 marks) For each case, choose one mutex for the list, one mutex per node, or a read-write lock, and justify:
    (a) a phone directory that is looked up thousands of times a second and changed a few times a day;
    (b) a list where almost every operation inserts or deletes;
    (c) a very long list where threads mostly work in different regions, writes are common, and memory is not a concern.
20. `[proof][L04]` (5 marks) In hand-over-hand Delete, the thread unlinks node `c` while holding the locks of `pred` and `c`, then unlocks `c` and frees it. (a) Why is it safe to free `c`? (b) Why can hand-over-hand locking never deadlock?
21. `[L04]` (4 marks) L04 slide 43 shows that with 80% Member / 10% Insert / 10% Delete the read-write lock is no faster than one mutex for the whole list. Explain why. Then explain why one mutex per node is slower than both.

## D. Code

22. `[code][L04]` (6 marks) From memory, write a C function `long threaded_sum(const long *a, long n, int p)` that sums an array with `p` Pthreads: a struct per thread, local partial sums, one mutex-protected addition per thread, and joins. (Then check it with exercise 5.)
23. `[code][L04]` (4 marks) Write `Member` for the read-write-lock list, and say which lock function `Insert` must call and why.
