# 13 · Quiz: MPI

Answer on paper first, as you would in the exam, then check
[`answers/13-mpi/quiz-answers.md`](../../answers/13-mpi/quiz-answers.md).
Marks in brackets show how much detail is expected. Use the signatures in
README section 5 as your "exam appendix". Tags: `[L10]` lecture, `[EXTRA]`
beyond the notes, `[tf]`, `[output]`, `[trace]`, `[calc]`, `[design]`, `[code]`.

## A. True or false? Justify briefly. (2 marks each)

1. `[tf][L10]` Every MPI process can read the global variables of rank 0, because they all run the same program.
2. `[tf][L10]` `MPI_Recv` returns immediately if no matching message has arrived yet.
3. `[tf][L10]` `MPI_Send` always waits until the receiver has received the message.
4. `[tf][L10]` If rank 1 sends message A and then message B to rank 0 (same tag and communicator), rank 0 may receive B first.
5. `[tf][L10]` After `MPI_Reduce(..., MPI_SUM, 0, MPI_COMM_WORLD)`, every rank holds the sum.
6. `[tf][L10]` Collective calls are matched using their tags.
7. `[tf][L10]` It is fine to write `if (my_rank == 0) MPI_Bcast(&n, 1, MPI_INT, 0, MPI_COMM_WORLD);`.
8. `[tf][EXTRA]` Replacing every `MPI_Send` by `MPI_Isend` (without any wait) is a safe way to avoid deadlock.

## B. Outputs and traces

9. `[output][L10]` (3 marks) Every rank runs `printf("rank %d\n", my_rank);` with `mpiexec -n 3`. Is the output `rank 2 / rank 0 / rank 1` possible? Is `rank 0 / rank 0 / rank 1` possible? Explain.
10. `[output][L10]` (3 marks) Rank 0 collects one message from each other rank with `MPI_Recv(..., MPI_ANY_SOURCE, ...)` and prints the sender's rank from the status. With 4 processes, list two different possible outputs. What changes if it receives from `source = q` for q = 1, 2, 3?
11. `[trace][L10]` (4 marks) Draw the tree global sum for 6 processes holding the values `3 1 4 1 5 9` (rank 0 to rank 5). For each round, list the messages (who sends to whom) and the partial sums. How many rounds and messages are needed?
12. `[trace][L10]` (3 marks) Draw the butterfly for 4 processes holding `5 2 1 3`. After each round, what does every rank hold? How many messages in total, compared with the tree?
13. `[trace][EXTRA]` (3 marks) Two ranks each run `MPI_Send(big, 1000000, MPI_DOUBLE, other, 0, comm); MPI_Recv(...)`. Describe what happens, and why the same code works with a count of 10. Give two fixes.

## C. Calculations and design

14. `[calc][L10]` (3 marks) n = 23 elements, p = 5 processes. Using block partitioning (the first n % p ranks get one more), which elements does each rank own? Using cyclic partitioning, which elements does rank 3 own?
15. `[calc][L10]` (3 marks) Rank 0 adds p numbers by receiving p − 1 messages one by one. Each message costs 1 µs and each addition 0.01 µs. Compare the time for p = 1024 with the tree global sum.
16. `[design][L10]` (4 marks) Work per element grows with the element's index (element i takes time proportional to i). Compare block and cyclic partitioning for p = 4 and n = 100: estimate each rank's work in both cases. Which do you choose and why?
17. `[design][L10]` (3 marks) Every rank needs the global maximum of an array that is spread over all ranks. Give two ways to do it with collectives, and say which is better and why.

## D. Code and outlines

18. `[code][L10]` (10 marks) **Outline an MPI program** that computes S = Σ f(i) for i = 1..N, where f is a given function (take f(i) = 1 / (i · i) as an example) and N is read from the keyboard by rank 0 only. The result must be printed by rank 0. Use the MPI functions correctly (all arguments), handle N that is not a multiple of the number of processes, and justify your partitioning.
19. `[code][EXTRA]` (4 + 6 marks) A program contains
    ```c
    a[0] = 1;
    for (i = 1; i < N; i++) a[i] = a[i-1] + 2*i;
    ```
    (a) Remove the loop-carried dependence. (b) Outline an MPI program in which each process computes its block of `a`, and rank 0 ends up printing `a[N-1]` and the sum of all elements of `a`.
20. `[code][L10]` (4 marks) Write the statements that let rank 0 send an array `double v[100]` to every other rank (a) with point-to-point calls, (b) with one collective call. Why is (b) usually faster?
21. `[code][L10]` (3 marks) Rank 0 has `int all[4 * p]`. Write the call that gives each rank 4 elements in `int mine[4]`, and the call that puts them back on rank 0 afterwards.
