# 16 · Quiz answers: one problem, four paradigms

These answers are about the length a marker expects for the marks shown.
"Why" is always part of the answer. Numbers marked "measured" come from the
exercises' tests (N = 10⁶ or 2·10⁶).

## A. True / false

1. **False.** Items can cost very different amounts. In the twin-prime
   count an even number costs 1 test and a prime near 10⁶ about 500. With a
   block split of 10⁶ items over 4 threads, the last thread does 2.2 times
   the work of the first (measured: 5.7 vs 12.5 million tests).
2. **False.** Cyclic breaks the "cost grows along the range" problem, but
   it can line up with patterns in the data. With 4 threads, threads 0 and
   2 get only even numbers, so cyclic is *worse* than block here (measured
   imbalance 1.98 vs 1.32).
3. **False.** Chunk size 1 means one access to the shared counter per
   iteration. For cheap iterations that overhead (and the contention
   between threads on one memory location) can cost more than the
   imbalance it removes. A moderate chunk (or `guided`) is usually better.
4. **True.** Chunks of 1 iteration are dealt to the threads in order: thread
   t gets iterations t, t + T, t + 2T, … Exercise 3's test checks this.
5. **False.** Two threads can both read the same `value` before either
   writes it back, so both get the same number (one number is tested twice
   and one is skipped). L01 slides 23–26 make exactly this point: the
   read and the increment must happen as one atomic step (an atomic
   fetch-and-add, or a lock).
6. **False.** The threads of a warp run in lock-step. If one thread tests a
   prime (hundreds of divisions) and its neighbours test even numbers (1
   test), the whole warp waits for the slow one: divergence is load
   imbalance at the warp level.

## B. Calculations

7. Average = (10 + 12 + 14 + 20) / 4 = 14. Imbalance = 20 / 14 ≈ **1.43**.
   Time is set by the slowest thread (20 units); one thread alone needs 56,
   so the speedup is 56 / 20 = **2.8**. Perfect balance: 14 units each,
   speedup **4**.
8. With 4 threads, thread t gets the numbers with p % 4 = t. For t = 0 and
   t = 2 these are all **even**, and every even number is rejected after one
   test; all primes (the expensive items) go to threads 1 and 3. Any **odd**
   number of threads mixes even and odd numbers in every thread, which
   avoids this particular problem (measured: 5 threads 1.28, 7 threads 1.18,
   still not perfect because of other patterns). With **6** threads,
   threads 0, 2, 4 get even numbers and thread 3 gets multiples of 3, all
   cheap. Primes only land on threads 1 and 5, and for p % 6 = 1 the number
   p + 2 is divisible by 3 (cheap second test), so thread 5 does most of
   the work: measured imbalance **3.20**.
9. One process only hands out work, so at most 3 processes compute: best
   speedup **3** with 4 processes (75% of the machine), **31** with 32
   (97%). Letting the manager compute is worth it when there are few
   processes, but then it answers requests late (workers wait while it is
   busy), so it should take small pieces of work and check for requests
   often (for example with `MPI_Iprobe`, [EXTRA]).
10. Useful work: 10⁶ × 400 ns = 0.4 s. `dynamic, 1`: 10⁶ atomic operations ×
    50 ns = 0.05 s, about **12.5%** overhead (more in practice, because
    threads fight over the counter's cache line). `dynamic, 100`: 10⁴
    operations × 50 ns = 0.5 ms, about **0.125%**, while 100 iterations per
    chunk are still small enough to balance well.

## C. Design and justification

11. *(5 marks: why uneven 1, what goes wrong 1, scheme 1, why it fixes it 1,
    cost/chunk 1.)* Trial division costs about √p / 2 divisions for a prime
    p, but only 1 for an even number and a few for numbers with small
    factors. So the work per item is very uneven and grows with p. A block
    split gives the thread with the largest numbers the most work (32% above
    average with 4 threads); a cyclic split with 4 threads gives two
    threads only even numbers (imbalance close to 2). I use **dynamic
    scheduling with chunks**: threads repeatedly take the next 64 numbers
    from a shared counter with an atomic fetch-and-add (`schedule(dynamic,
    64)` in OpenMP). A thread that happens to get expensive numbers simply
    takes fewer chunks, so all threads finish within about one chunk of
    each other (measured imbalance about 1.03). The chunk size keeps the
    counter overhead to one atomic operation per 64 numbers, which is tiny
    next to the divisions. (Block-cyclic with blocks of 1000 is a good
    static alternative: measured 1.00.)
12. Neighbouring threads of a warp test neighbouring numbers, so in every
    warp a few threads (primes) do hundreds of divisions while the others
    stop after one; the warp runs until its slowest thread finishes, with
    most lanes idle. Simple fix: **skip the even numbers** (give thread i
    the odd number 2i + 1), which removes the cheapest half of the items
    and makes neighbours more alike. Going further, a group of threads can
    share the divisions of one number.
13. MPI processes have **no shared memory**, so there is no variable they
    could all fetch-and-add. Instead one process (the manager) owns the
    counter and hands out chunks in messages: a worker sends "ready" and
    receives the start of its next chunk (`MPI_ANY_SOURCE` lets the manager
    serve whoever asks first). Every chunk costs a message round trip
    (microseconds, much more across a network), against tens of
    nanoseconds for an atomic, so MPI chunks must be **much larger** (we use
    1000 items).
14. - **Pthreads:** each thread counts privately and stores its count;
      main adds them after `pthread_join`. Naive: `total++` on a shared
      variable from all threads is a race (lost updates); a mutex around
      every update is correct but serializes the loop.
    - **OpenMP:** `reduction(+:count)`. Naive: `count += ...` without it is a
      race; `#pragma omp critical` around it is correct but slow.
    - **MPI:** `MPI_Reduce(&count, &total, 1, MPI_LONG, MPI_SUM, 0,
      MPI_COMM_WORLD)`, called by all ranks. Naive: assuming rank 0 can
      read the others' variables; or calling Reduce only on rank 0 (hangs).
    - **CUDA:** a private counter per thread, then one `atomicAdd` per
      thread (or a block reduction); then `cudaMemcpy` back. Naive:
      `*total += 1` in the kernel is a race; one `atomicAdd` per item is
      correct but all threads queue on one address; forgetting to zero the
      counter (`cudaMemset`) or to copy it back.

## D. Outlines

15. ```c
    long N, M;                          // M = N - 1 items: p = 0 .. N-2
    atomic_long next = 0;               // SHARED: the only shared variable

    void *worker(void *arg) {
        long *result = arg;
        long count = 0;                 // PRIVATE
        for (;;) {
            long start = atomic_fetch_add(&next, CHUNK);   // take a chunk, atomically
            if (start >= M) break;
            long end = min(start + CHUNK, M);
            for (long p = start; p < end; p++)
                if (is_prime(p) && is_prime(p + 2)) count++;
        }
        *result = count;                // written once
        return NULL;
    }

    main: read N; M = N - 1;
          for t in 0..T-1: pthread_create(&th[t], NULL, worker, &res[t]);
          total = 0;
          for t in 0..T-1: { pthread_join(th[t], NULL); total += res[t]; }
          print total
    ```
    Shared: only `next`, updated with an atomic fetch-and-add (L01's
    getAndIncrement). Each thread's count is private, so no lock is needed;
    results are read only after `pthread_join`, which guarantees the thread
    has finished writing. Chunks of, say, 64 balance the uneven cost of the
    items (question 11) with little overhead.
16. ```c
    long count = 0;
    #pragma omp parallel for reduction(+:count) schedule(dynamic, 64)
    for (long p = 0; p <= N - 2; p++)
        if (is_prime(p) && is_prime(p + 2)) count++;
    ```
    `parallel for`: split the iterations among a team of threads (they
    are independent). `reduction(+:count)`: each thread gets a private
    `count` starting at 0, and the copies are added at the end, so there is
    no race. `schedule(dynamic, 64)`: threads take chunks of 64 iterations
    when they are free, which balances the uneven work (`guided` would also
    be a good answer).
17. *(8 marks: setup/Bcast 1, manager loop 2, worker loop 2, termination
    1, Reduce + output 2.)*
    ```c
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &p);  MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    if (my_rank == 0) read N;
    MPI_Bcast(&N, 1, MPI_LONG, 0, MPI_COMM_WORLD);         // everyone needs N
    M = N - 1;  count = 0;

    if (my_rank == 0) {                                    // MANAGER: hands out chunks
        next = 0;  stopped = 0;
        while (stopped < p - 1) {
            MPI_Recv(&dummy, 1, MPI_INT, MPI_ANY_SOURCE, TAG_READY, MPI_COMM_WORLD, &status);
            if (next < M) { start = next; next += CHUNK; }
            else          { start = -1;  stopped++; }      // no work left: tell this worker to stop
            MPI_Send(&start, 1, MPI_LONG, status.MPI_SOURCE, TAG_WORK, MPI_COMM_WORLD);
        }
    } else {                                               // WORKER
        while (true) {
            MPI_Send(&dummy, 1, MPI_INT, 0, TAG_READY, MPI_COMM_WORLD);
            MPI_Recv(&start, 1, MPI_LONG, 0, TAG_WORK, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            if (start < 0) break;
            for (q = start; q < min(start + CHUNK, M); q++)
                if (is_prime(q) && is_prime(q + 2)) count++;
        }
    }
    MPI_Reduce(&count, &total, 1, MPI_LONG, MPI_SUM, 0, MPI_COMM_WORLD);   // ALL ranks; manager adds 0
    if (my_rank == 0) printf("%ld\n", total);
    MPI_Finalize();
    ```
    Every worker gets exactly one −1, so the manager's loop ends after
    p − 1 stops and no message is left unreceived. `MPI_ANY_SOURCE` serves
    whichever worker is free first: that is what balances the load. CHUNK
    is large (e.g. 1000) because each chunk costs a message round trip.
    (A static alternative: `for (q = my_rank; ...; q += p)` with no manager,
    but with an even p this cyclic split gives some ranks only even
    numbers, so use block-cyclic chunks.)
18. ```c
    __device__ int is_prime(long n) { ... trial division ... }

    __global__ void twins_kernel(long N, unsigned long long *total) {
        long first = (long)blockIdx.x * blockDim.x + threadIdx.x;
        long step  = (long)gridDim.x * blockDim.x;       // grid-stride: any N
        unsigned long long mine = 0;                     // private, in a register
        for (long p = first; p <= N - 2; p += step)
            if (is_prime(p) && is_prime(p + 2)) mine++;
        if (mine > 0) atomicAdd(total, mine);            // combine safely, rarely
    }

    host:
        cudaMalloc((void **)&d_total, sizeof(unsigned long long));
        cudaMemset(d_total, 0, sizeof(unsigned long long));
        twins_kernel<<<128, 256>>>(N, d_total);
        check cudaGetLastError(); cudaDeviceSynchronize();
        cudaMemcpy(&total, d_total, sizeof total, cudaMemcpyDeviceToHost);
        cudaFree(d_total);
        print total
    ```
    No input array needs copying: each thread computes its numbers from its
    index. The private counter plus one `atomicAdd` per thread avoids both
    the race and heavy contention. Remark: divergence inside warps is this
    version's load-balancing problem; testing only odd numbers halves it.
