# 04 · Quiz answers: OpenMP

These answers are about the length a marker expects for the marks shown.
"Why" is always part of the answer.

## A. True / false

1. **False.** Without `-fopenmp`, gcc treats `#pragma omp` lines as unknown
   and ignores them. The program builds and runs on one thread. (Calls to
   `omp_...` functions then fail at the link step, but a program with only
   pragmas builds fine.) With `-Wall` you get a warning, nothing more.
2. **False.** L04 slide 50: the standard does not guarantee the requested
   number. The system may have limits. Code must use
   `omp_get_num_threads()` to find the real team size.
3. **False.** Variables declared inside the region are private: each thread
   has its own copy. Variables declared before the region are shared.
4. **False.** `atomic` applies to one simple update statement such as
   `x++` or `x += e`, done as one hardware operation. For a block, use
   `critical`.
5. **True.** `static` divides the iterations before the loop starts, using
   only the number of iterations and threads, so the mapping is the same
   every time. (`dynamic` and `guided` depend on timing.)
6. **False.** Each thread adds into its own private copy, with no lock.
   The private copies are combined only once per thread, at the end. That
   is why it is fast.
7. **False.** Every thread runs the whole loop, so the work is done once per
   thread (and if the loop writes shared data, the threads race). To share
   the iterations you need `#pragma omp for` inside the region, or
   `#pragma omp parallel for`.
8. **False.** `single` has an implied barrier at its end; `master` does not.
   With `master`, the other threads skip the block and carry on at once.

## B. Short answer

9. (a) and (b) are possible: the threads run independently, so they can
   print in any order (L04 slide 47). (c) is impossible: thread numbers are
   unique, so exactly one thread prints 0, one prints 1, one prints 2.
10. (a) **10.** `firstprivate` gives each thread its own copy starting at
    10. The threads change only their copies. The outside `x` is never
    touched.
    (b) **13** = 10 + 0 + 1 + 2. `y` is shared, and `atomic` makes each
    `y += id` one indivisible step, so no update is lost.
    (c) **10, 11, 12 or 13.** `y += id` is load, add, store. A thread can
    load an old value and store after others, wiping out their updates. For
    example: thread 0 loads 10; thread 1 loads 10 and stores 11; thread 2
    loads 11 and stores 13; thread 0 finally stores 10 + 0 = 10. The
    explorer confirms that exactly these four values are possible.
11. (a) `static`: one block per thread, sizes 3, 3, 2, 2 (GCC gives the
    extra iterations to the first threads): thread 0 runs 0–2, thread 1 runs
    3–5, thread 2 runs 6–7, thread 3 runs 8–9. Iteration 7: **thread 2**.
    (b) `static, 2`: blocks of 2 dealt out in turn: {0,1}→0, {2,3}→1,
    {4,5}→2, {6,7}→3, {8,9}→0. Iteration 7: **thread 3**.
    (`make run M=04 EX=schedule_map ARGS="10 4"` shows both.)
12. (a) **static**: every row costs the same, so equal blocks are already
    balanced, and static has no run-time cost for handing out work.
    (b) **dynamic with a chunk such as 64, or guided**: testing a big number
    takes longer than a small one, so with static the thread with the last
    block would work much longer while others sit idle. Chunks above 1 keep
    the hand-out cost low.
    (c) **dynamic** (chunk 1 or small): the cost of a row is very uneven
    and unpredictable, so threads should take a new row whenever they are
    free. There are few rows, so the hand-out cost is small.
13. Fastest to slowest: **reduction**, **per-thread counters** (about the
    same; slower if the counters share a cache line), **atomic**,
    **critical**. `critical` takes a lock for every hit (millions of times),
    so threads mostly wait. `atomic` needs no lock but all threads still
    update one memory location millions of times, which the hardware must
    pass between cores. `reduction` and per-thread counters update private
    memory and combine only once per thread. `reduction` is exactly Lab 01
    Ex 3 Part B (count locally, add once under a lock), done automatically.
14. All threads of a team run the parallel region. Without `single`, every
    thread calls the sort on the whole array: the array is sorted `p` times
    at once, on the same memory and the same temporary array, so threads
    overwrite each other's merges (wrong results) and the work is multiplied.
    With `single`, one thread starts the recursion, and the others pick up
    the tasks it creates.
15. It gets much slower, often slower than the serial sort. Every task
    costs time to create, queue and hand to a thread, and a task that sorts
    one or two numbers does far less work than that. The amount of work per
    task (its **granularity**) must be large enough to pay for the overhead.
    On our machine, 100,000 numbers with tasks all the way down took up to
    14 times longer than the serial sort.
16. With `private(x)` each thread's `x` is a new variable with **no defined
    starting value** (you may see 0 or garbage; reading it is a bug). After
    the region, `x` has its old value, because the region changed only the
    private copies. With `firstprivate(x)`, each copy **starts as the
    outside value**; after the region, `x` is again unchanged.
17. Blocks: `[2 4] [1 3] [5 1]`. Pass 1 (each block from 0): `2 6 | 1 4 | 5 6`.
    Block totals: 6, 4, 6. Offsets (sum of earlier blocks): 0, 6, 10.
    Pass 2 (add the offset): `2 6 | 7 10 | 15 16`. Result `2 6 7 10 15 16`,
    which matches the serial prefix sum.

## C. Can this loop run in parallel?

18. (a) **Safe.** Each iteration writes only `a[i]` and reads only `b[i]`
    and `c[i]`.
    (b) **Not safe as written:** iteration `i` reads `a[i-1]`, written by
    iteration `i-1` (a loop-carried dependence). Fix with the closed form
    `a[i] = 1 + 2*i`, which needs only `i`.
    (c) **Not safe as written:** `t` is declared before the loop, so it is
    shared, and threads overwrite each other's `t` between the two lines.
    The iterations are really independent (each swaps only position `i`).
    Fix: declare `t` inside the loop body, or add `private(t)`.
    (d) **Not safe:** iteration `i` must read the **old** `a[i+1]`, but
    iteration `i+1` overwrites `a[i+1]`. In parallel, iteration `i+1` may
    run first, so iteration `i` reads the new value. Fix: copy the array
    first (`old[i] = a[i]` in one parallel loop), then
    `a[i] = old[i+1] + 1` in a second parallel loop.
    (e) **Not parallelisable:** each step needs the previous `p`, and for a
    general `f` there is no formula or reordering. It must run in order.
    (f) **Not safe as written** (two threads can read and write `best` at
    once and lose the larger value), but it is a maximum, so use
    `reduction(max:best)`.
19. Values: 1, 4, 9, 16, 25… so `a[i] = (i+1)*(i+1)` (the sum of the first
    `i+1` odd numbers is a square). Each `a[i]` now depends only on `i`, not
    on `a[i-1]`, so the iterations are independent and can run in any order
    on any thread.

## D. Code and design

20. ```c
    double trap(double a, double b, long n) {
        double h = (b - a) / n, total = 0.0;            // total is shared
    #pragma omp parallel
        {
            int id = omp_get_thread_num(), p = omp_get_num_threads();
            long first = (long)id * n / p, last = (long)(id + 1) * n / p;
            double mine = 0.0;                           // private
            for (long k = first; k < last; k++)
                mine += (f(a + k*h) + f(a + (k+1)*h)) / 2.0 * h;
    #pragma omp critical
            total += mine;
        }
        return total;
    }
    ```
    One critical section per thread means only `p` lock operations. One per
    trapezoid would mean `n` of them (millions), and the threads would spend
    their time queueing for the lock, slower than the serial program.
21. ```c
    double h = (b - a) / n, s = (f(a) + f(b)) / 2.0;
    #pragma omp parallel for reduction(+:s)
    for (long i = 1; i < n; i++) s += f(a + i*h);
    return s * h;
    ```
    Each thread gets a private `s` starting at 0; OpenMP adds the private
    sums into `s` at the end.
22. ```c
    void sort_rec(int *a, int *tmp, int lo, int hi, int depth, int limit) {
        if (lo >= hi) return;
        if (depth >= limit) { merge_sort_serial(a, tmp, lo, hi); return; }   // cutoff
        int mid = lo + (hi - lo) / 2;
    #pragma omp task
        sort_rec(a, tmp, lo, mid, depth + 1, limit);
    #pragma omp task
        sort_rec(a, tmp, mid + 1, hi, depth + 1, limit);
    #pragma omp taskwait          // both halves must be sorted before we merge them
        merge(a, tmp, lo, mid, hi);
    }
    void merge_sort_parallel(int *a, int *tmp, int n, int limit) {
    #pragma omp parallel          // create the team
    #pragma omp single            // one thread starts; the others run the tasks
        sort_rec(a, tmp, 0, n - 1, 0, limit);
    }
    ```
    `taskwait` goes between the two tasks and the `merge`. Without it,
    `merge` could run while the halves are still being sorted.
23. `rand()` keeps one hidden state shared by all threads. Calling it from
    several threads at once is a race on that state: values can repeat or be
    corrupted, and the calls are slow because the threads fight over it.
    `rand_r(&seed)` keeps its state in the variable you pass. A seed declared
    inside the region is private, so each thread has its own random
    sequence and no race. (Different seeds per thread, such as
    `42 + thread number`, make the sequences different.)
