# 11 · Quiz answers: parallel patterns & design

These answers are about the length a marker expects for the marks shown.
"Why" is always part of the answer.

## A. True / false

1. **False.** Items can cost very different amounts. In L01's prime example
   (slides 17–19) big numbers take longer to test, so the thread with the
   biggest numbers does the most work and finishes last.
2. **False.** Dynamic scheduling pays for every hand-out (an atomic operation
   on a shared counter, plus worse cache use). When every iteration costs the
   same, like the blur filter, static is balanced anyway and avoids that cost.
3. **False.** One item still goes through all 4 stages one after another, so
   it takes as long as before (a little longer, because of the queues). A
   pipeline improves *throughput*: once it is full, one item comes out per
   stage time instead of one per 4 stage times.
4. **False.** Only 4 threads can run at the same moment. The other 12 just
   wait for a core, and switching between threads adds overhead and cache
   misses. For CPU-bound work, about one thread per core is best.
5. **True.** Item *i* belongs to thread *i mod p* and item *i + 1* to thread
   *(i + 1) mod p*. With *p* > 1 these always differ. (That is why cyclic is
   bad when neighbouring items are used together.)
6. **False.** Thieves take from other workers' deques, so each deque is still
   shared. In Lab 06 each deque has its own mutex. The gain is that the owner
   almost never *competes* for its lock, not that there is no lock.
7. **True.** A race needs at least one thread that writes. If the table is
   fully loaded before the threads start (`pthread_create` makes the earlier
   writes visible), every thread can read it freely.

## B. Short answer and calculations

8. (a) 14 / 4 = 3, 14 mod 4 = 2, so threads 0 and 1 get 4 items and threads 2
   and 3 get 3: **[0, 4), [4, 8), [8, 11), [11, 14)**.
   (b) Cyclic: thread *r* gets 14 / 4 = 3 items plus one if *r* < 2:
   **4, 4, 3, 3**.
   (c) Item 13 is in block 13 / 3 = 4, and block 4 goes to 4 mod 4 =
   **thread 0**. (Check: `make run M=11 EX=ex1_partition ARGS="14 4 3" ANS=1`.)
9. (a) **Yes.** Each iteration uses only its own elements.
   (b) **No.** Iteration *i* needs `a[i-1]` from iteration *i − 1* (a
   loop-carried dependence). Some such loops can be redone with a parallel
   prefix ("scan") algorithm, but not by just splitting the loop.
   (c) **Yes, as a reduction.** As written, every iteration reads and writes
   `best`: a race. But max can be regrouped: each thread finds the max of its
   part, then the maxima are combined (`reduction(max:best)` in OpenMP).
   (d) **Yes.** Iterations write only even indices and read only odd ones, so
   no iteration reads what another writes.
   (e) **No, as written.** Two iterations with equal `a[i]` update the same
   `count` element at the same time and lose updates. Fix: an atomic
   increment, or one private histogram per thread added together at the end
   (better: no contention).
10. (a) 50 × (4 + 12 + 4) = **1000 ms**.
    (b) Fill time 4 + 12 + 4 = 20 ms, then one item every 12 ms (the slowest
    stage): 20 + 49 × 12 = **608 ms**.
    (c) One item per 12 ms = **about 83 items per second**.
    (d) Three threads finish 3 items per 12 ms, i.e. one per 4 ms. All stages
    now run at 4 ms: 20 + 49 × 4 = **216 ms**, **250 items per second**.
    (e) **No.** Stage 2 would deliver one item per 3 ms, but stages 1 and 3
    still need 4 ms per item and are now the bottleneck. The pipeline is only
    as fast as its slowest stage.
    (Check: `make run M=11 EX=pipeline_calc ARGS="50 4 12 4"` and `ARGS="50 4 12/3 4"`.)
11. Equal stages: total = (k + n − 1) × t = (5 + 20 − 1) × 3 = **72 s**. One
    machine: 20 × 5 × 3 = 300 s. Speedup = 300 / 72 ≈ **4.2**. It approaches
    5 (the number of stages) only when there are many more items than stages,
    because the 4 extra fill steps are spread over more items.
12. **Too small:** tens of thousands of threads are created. Creating and
    joining a thread costs tens of microseconds, more than summing 100 nodes,
    so the overhead is bigger than the useful work. Too many threads also
    compete for the cores. **Too large:** only a handful of subtrees are big
    enough to get a thread, so only a few cores work while the rest sit idle.
    The tree is unbalanced, so those few pieces are also of very different
    sizes and the biggest one dominates. **Middle value:** a few times more
    pieces than cores, each big enough to hide the cost of its thread.
13. (a) Thread 0 gets tasks 0–9: 4 × 100 + 6 × 1 = **406 ms**. The others
    take 10 ms each. The program finishes at 406 ms.
    (b) The four threads take tasks 0, 1, 2 and 3 first: one 100 ms task
    each. Then they share the 36 short tasks, 9 each: **about 109 ms**.
    (c) One thread: 400 + 36 = 436 ms. Static: 436 / 406 ≈ **1.07**. Shared
    queue: 436 / 109 = **4.0**. Pick the task queue: it hands out work as
    threads become free, so the long tasks end up on different threads
    without anyone having to predict which tasks are long.
14. (1) **Less fighting:** owner and thief work at opposite ends, so they want
    the same task only when one is left; most of the time the owner's lock is
    free. (2) **Cache:** the newest task, at the back, usually uses data the
    owner touched a moment ago, which is still in its cache. (3) Also: in
    divide & conquer the oldest tasks, at the front, are the biggest, so one
    steal gives the thief a lot of work and steals stay rare.
15. **Clearly faster** with many threads and many *small* tasks: every task
    from a shared queue means one more trip through the same mutex, and the
    threads queue up for it. Also when tasks create new tasks, which a worker
    can keep locally. **Small difference** with few threads and long tasks
    (like Lab 06's tasks of thousands of steps): the queue's lock is taken
    rarely compared with the work, so it is almost never contended.
16. One stage-2 thread takes the pill, forwards it and stops. The other two
    wait forever in `take()` on the empty queue `q12`. Stage 3 receives one
    pill; if it waits for three it waits forever, and if it stops after one
    it may miss items still being processed by the other two threads. Either
    way `pthread_join` never returns: the program **hangs**. Stage 1 must send
    **3 pills, one per stage-2 thread**. Each stage-2 thread forwards its pill
    to stage 3, and stage 3 stops after it has counted **3 pills**. Each
    thread forwards its pill only after its last real item, so stage 3 has
    received every item by then.
17. Every blur pixel costs about the same, so a static split already gives
    every thread the same work, and static has no run-time cost. Mandelbrot
    pixels cost anywhere from 1 to `max_iter` steps, and the heavy rows are
    bunched together in the middle of the picture. A static block split gives
    those rows to one or two threads while the others finish early and idle.
    Dynamic scheduling hands rows out as threads become free, so the heavy
    rows are spread out.

## C. Choose a pattern

18. (a) **Pipeline** (scan → resize → store), with the **resize stage
    replicated** (about 5 threads). Every photo goes through the same fixed
    steps in order, and the stages can work on different photos at the same
    time. Throughput is set by the slowest stage, so that is the one that
    needs more threads. (A thread pool where each task does all three steps
    is also acceptable, if justified.)
    (b) **Map-reduce / loop parallel with chunks.** Every reading is
    independent and the answer is one combined value. Read the file in
    chunks (it may not fit in memory). Each thread finds the maximum of its
    part in a private variable, and the maxima are combined at the end
    (reduce). No lock per reading.
    (c) **Work stealing** (divide & conquer over an irregular tree). Tasks
    create new tasks, and nobody knows in advance how big a branch is. Each
    worker keeps the tasks it creates on its own deque and idle workers
    steal, so the work spreads out without one shared queue becoming a
    bottleneck.
    (d) **Loop parallel with a static split.** Pixels are independent and
    cost the same, so a block split is already balanced and has no run-time
    cost. Blocks of rows also use the cache well.
    (e) **Thread pool with a shared task queue** (producer/consumer). Jobs
    arrive over time and vary a lot in size. A fixed set of workers takes the
    next job when free, so big jobs do not hold up the small ones behind
    them, and no thread is created per job.

## D. Design justification

19. The thread computes for 2 ms out of every 20 ms. While it waits, the core
    can run other threads. Rule of thumb: threads ≈ cores × (1 + wait /
    compute) = 8 × (1 + 18 / 2) = **80 threads**. Fewer leaves cores idle while
    requests wait for the database; many more just adds switching and memory
    use. (The database itself may also limit how many requests can usefully
    wait at once.)
20. (a) **Process it in chunks** of, say, 1 GB with one reusable buffer, so
    memory stays bounded. All threads work on the current chunk (split by
    lines). A chunk usually ends in the middle of a line: process up to the
    last newline and carry the rest over to the next chunk. Optionally, read
    the next chunk while the threads work on the current one.
    (b) Every write takes the same mutex, so only one thread writes at a time
    and the others queue up. The writing has become a serial bottleneck
    (Amdahl, L02; the code under a lock runs one thread at a time, L12 slide
    5). Give each thread its **own buffer** (or temporary file), write in big
    blocks, and merge the buffers at the end, which also gives a predictable
    order.
    (c) **Load it once, before starting the threads**, and share one copy.
    Nobody writes it, so it needs **no lock** (a race needs a writer).
    Loading it per task wastes time and 200 MB per copy; loading it lazily
    inside the threads would need extra synchronization.
21. ```
    shared next = 0
    worker(rank):
        loop:
            start = fetch_and_add(next, c)      // returns the OLD value
            if start >= n: stop
            for i in start .. min(start + c, n) - 1:
                body(i)
    ```
    Fetch-and-add reads and adds in **one atomic step**. With a separate read
    and write, two threads could both read the same `next` and both run the
    same chunk, while another chunk could be skipped (the lost update of
    `counter++`, L01 slides 22–24).
22. ```
    stage2 (k copies):
        loop:
            item = take(q12)
            if item is POISON: put(q23, item); stop
            item.value = work(item.value)
            put(q23, item)

    stage3:
        next = 0; pills = 0
        while pills < k:
            item = take(q23)
            if item is POISON: pills++; continue
            waiting[item.seq] = finish(item)    // hold back early items
            while waiting[next] is filled:
                emit(waiting[next]); next++
    ```
    Items carry a sequence number because two stage-2 threads can overtake
    each other. Stage 3 stops only after one pill from each of the *k*
    stage-2 threads, so no item is still on its way.
