# 13 · Quiz answers: MPI

These answers are about the length a marker expects for the marks shown.
"Why" is always part of the answer.

## A. True / false

1. **False.** Each MPI process is a separate copy of the program with its
   **own memory** (distributed memory, L10 slide 2). Running the same code
   does not mean sharing variables. Data moves only in messages.
2. **False.** `MPI_Recv` **blocks** until a matching message has arrived
   (slide 21).
3. **False.** It depends on the MPI library and the message size
   (slide 21). Small messages are usually copied into a buffer and
   `MPI_Send` returns at once; large ones make it wait for the receiver.
   A correct program must work either way.
4. **False.** Messages from the same sender to the same receiver, on the
   same communicator, are **not overtaken**: they arrive in the order sent
   (slide 21). (With *different* tags, a receive asking for B's tag could
   take B first, but that is the receiver choosing, not overtaking.)
5. **False.** Only the **root** (here rank 0) gets the result in its
   `recvbuf`. Use `MPI_Allreduce` if every rank needs it (slide 37).
6. **False.** Collectives have no tags. They are matched by the
   communicator and the **order** in which the processes call them
   (slide 36).
7. **False.** A collective must be called by **every** process of the
   communicator (slide 35). Here only rank 0 calls it, so rank 0 waits for
   the others (or sends to nobody), and the other ranks never get `n`.
   Every rank must call `MPI_Bcast(&n, 1, MPI_INT, 0, MPI_COMM_WORLD)`;
   the root argument (0) says who sends.
8. **False.** `MPI_Isend` only *starts* the send. Without `MPI_Wait` you
   cannot know when the buffer may be reused or freed, and `MPI_Finalize`
   with unfinished requests is an error. The fix is Isend/Irecv **plus**
   `MPI_Wait`/`MPI_Waitall`.

## B. Outputs and traces

9. `rank 2 / rank 0 / rank 1`: **possible**. The three processes run at
   the same time and print independently; nothing orders them.
   `rank 0 / rank 0 / rank 1`: **impossible**. Each rank prints exactly
   once and the ranks are 0, 1, 2, so each number appears exactly once.
   (Lines are not split up here because each `printf` is one short line,
   but MPI does not even promise that in general.)
10. With `MPI_ANY_SOURCE` the messages are taken in the order they
    arrive (slide 17), so e.g. `1 2 3` or `3 1 2` (any of the 6 orders) are
    possible. With `source = q` in a loop q = 1, 2, 3, rank 0 waits for rank
    1 first even if rank 3's message is already there, so the output is
    always `1 2 3`. The first is faster when workers finish at different
    times; the second gives a fixed order.
11. Values `3 1 4 1 5 9` on ranks 0–5.
    - Round 0 (d = 1): 1→0 (3+1 = **4**), 3→2 (4+1 = **5**), 5→4 (5+9 = **14**).
    - Round 1 (d = 2): 2→0 (4+5 = **9**). Rank 4 would receive from rank 6,
      which does not exist, so it waits.
    - Round 2 (d = 4): 4→0 (9+14 = **23**).
    Check: 3+1+4+1+5+9 = 23. **3 rounds** (⌈log₂ 6⌉ = 3) and **5
    messages** (p − 1).
12. Values `5 2 1 3`.
    Round 0 (partners 0↔1, 2↔3): `7 7 4 4`.
    Round 1 (partners 0↔2, 1↔3): `11 11 11 11`.
    Every round every rank sends one message: 4 × 2 = **8 messages**,
    against 3 for the tree. Both take 2 rounds, but the butterfly leaves
    the total on **every** rank (that is an Allreduce); the tree would need
    2 more rounds of broadcast to do the same.
13. Each `MPI_Send` of 8 MB is too big for MPI's buffer, so it waits until
    the partner starts the matching receive. Both partners are in their
    `MPI_Send`, so neither ever reaches `MPI_Recv`: **deadlock**, the
    program hangs. With 10 doubles MPI copies the message into a buffer and
    `MPI_Send` returns at once, so both reach their receive. Fixes: (1)
    order by rank: even ranks send first, odd ranks receive first; (2) one
    `MPI_Sendrecv`; (3) `MPI_Isend` + `MPI_Irecv` then `MPI_Waitall`.
    (Exercise 9.)

## C. Calculations and design

14. q = 23 / 5 = 4, extra = 23 % 5 = 3. Ranks 0–2 get 5 elements, ranks 3–4
    get 4: rank 0: **0–4**, rank 1: **5–9**, rank 2: **10–14**, rank 3:
    **15–18**, rank 4: **19–22**. Cyclic, rank 3: **3, 8, 13, 18** (i % 5 == 3).
15. One by one: 1023 messages + 1023 additions ≈ 1023 × 1.01 µs ≈
    **1033 µs**. Tree: ⌈log₂ 1024⌉ = 10 rounds, each one message plus one
    addition on the busy ranks, all pairs at the same time: ≈ 10 × 1.01 ≈
    **10.1 µs**, about 100 times faster. The total number of messages is
    the same (1023); the gain comes from doing them in parallel.
16. Elements 0..99, element i costs i (total 4950, ideal 1237.5 per rank).
    **Block** (25 each): rank 0: 0+…+24 = 300; rank 1: 925; rank 2: 1550;
    rank 3: 2175. The job takes as long as rank 3: 2175, which is 1.76 times
    the ideal; rank 0 is idle 86% of the time. **Cyclic** (rank r gets
    r, r+4, …): rank r gets 25r + 4 × (0+…+24) = 1200 + 25r, so 1200, 1225,
    1250, 1275, almost perfectly even. **Choose cyclic**: when the cost
    grows along the array, dealing elements round-robin gives every rank a
    mix of cheap and expensive ones.
17. (a) `MPI_Allreduce(&local_max, &global_max, 1, MPI_DOUBLE, MPI_MAX,
    MPI_COMM_WORLD)`: one call, every rank gets the maximum. (b)
    `MPI_Reduce(..., MPI_MAX, 0, ...)` and then `MPI_Bcast(&global_max, 1,
    MPI_DOUBLE, 0, ...)`: two collectives. (a) is better: one call, and the
    library can use a butterfly that finishes in about half the rounds of
    Reduce + Bcast. (A third way, `MPI_Allgather` of all local maxima and
    then a local loop, sends more data than needed.)

## D. Code and outlines

18. *(10 marks: setup 1, input/Bcast 2, partitioning with leftovers 2,
    local loop 1, Reduce with correct arguments 2, output/finalize 1,
    justification 1.)*

    ```c
    #include <mpi.h>
    double f(long i) { return 1.0 / ((double)i * i); }

    int main(int argc, char **argv) {
        int my_rank, p;
        long N;
        MPI_Init(&argc, &argv);
        MPI_Comm_size(MPI_COMM_WORLD, &p);
        MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

        if (my_rank == 0) scanf("%ld", &N);            // only rank 0 can read the keyboard
        MPI_Bcast(&N, 1, MPI_LONG, 0, MPI_COMM_WORLD);  // ALL ranks call it; now everyone knows N

        // block partitioning of 1..N; the first N % p ranks get one extra item
        long q = N / p, extra = N % p;
        long count = q + (my_rank < extra ? 1 : 0);
        long first = 1 + my_rank * q + (my_rank < extra ? my_rank : extra);

        double local = 0.0, total = 0.0;
        for (long i = first; i < first + count; i++) local += f(i);   // no communication here

        // ALL ranks call it; sendbuf, recvbuf, count, type, op, root, communicator
        MPI_Reduce(&local, &total, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);

        if (my_rank == 0) printf("S = %.15f\n", total);   // only the root has the total
        MPI_Finalize();
        return 0;
    }
    ```
    **Justification.** Each f(i) costs the same, so block partitioning gives
    every rank the same amount of work (±1 item) and needs only two
    numbers (`first`, `count`) per rank. If the cost of f(i) grew with i
    (for example a primality test), cyclic partitioning (`for (i = 1 +
    my_rank; i <= N; i += p)`) would balance better (module 16). There is
    one Bcast and one Reduce in total, so communication is tiny compared
    with the computation.
19. **(a)** a[i] = 1 + 2·(1 + 2 + … + i) = **1 + i(i + 1)**. Check: a[1] =
    1 + 2 = 3 ✓, a[2] = 3 + 4 = 7 = 1 + 6 ✓. Every element can now be
    computed on its own, with no dependence on the previous one.
    **(b)**
    ```c
    MPI_Init(&argc, &argv);  MPI_Comm_size(MPI_COMM_WORLD, &p);  MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    if (my_rank == 0) read N;
    MPI_Bcast(&N, 1, MPI_LONG, 0, MPI_COMM_WORLD);
    q = N / p;  extra = N % p;                                 // block partitioning of 0..N-1
    count = q + (my_rank < extra);  first = my_rank * q + min(my_rank, extra);
    local_sum = 0;
    for (j = 0; j < count; j++) {
        i = first + j;
        a_local[j] = 1 + i * (i + 1);                          // closed form: no dependence
        local_sum += a_local[j];
    }
    MPI_Reduce(&local_sum, &total, 1, MPI_LONG_LONG, MPI_SUM, 0, MPI_COMM_WORLD);
    if (my_rank == 0) print 1 + (N-1)*N  and  total;           // a[N-1] by the formula, no message needed
    MPI_Finalize();
    ```
    Without a closed form you would use `MPI_Scan` on the block totals to
    find each rank's starting offset (README section 8.4, exercise 10). If
    the marker wants a[N-1] to come from the array, the rank that owns it
    (rank p−1 when N ≥ p) sends it to rank 0 with `MPI_Send`/`MPI_Recv`.
20. (a)
    ```c
    if (my_rank == 0)
        for (int q = 1; q < p; q++) MPI_Send(v, 100, MPI_DOUBLE, q, 0, MPI_COMM_WORLD);
    else
        MPI_Recv(v, 100, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    ```
    (b) every rank: `MPI_Bcast(v, 100, MPI_DOUBLE, 0, MPI_COMM_WORLD);`
    (b) is usually faster because the library can use a **tree**
    (slide 41): ranks that already have the data pass it on, so it takes
    about log₂ p rounds instead of p − 1 sends one after another from rank 0.
21. ```c
    MPI_Scatter(all, 4, MPI_INT, mine, 4, MPI_INT, 0, MPI_COMM_WORLD);   // 4 = items PER rank
    ... work on mine ...
    MPI_Gather(mine, 4, MPI_INT, all, 4, MPI_INT, 0, MPI_COMM_WORLD);
    ```
    Both are called by every rank; `all` only matters on rank 0.
