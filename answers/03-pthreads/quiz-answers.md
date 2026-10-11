# 03 · Quiz answers: Pthreads

These answers are about the length a marker expects for the marks shown.
"Why" is always part of the answer.

## A. True / false

1. **False.** Each thread has its own stack, and local variables live on
   the stack. So every thread that runs the function gets its own copy.
   Globals and `malloc`ed memory are shared.
2. **False.** `pthread_join(t)` waits for thread `t` only. If `t` has
   already finished, it returns at once. Joining in any order works; the
   last join returns once every thread is done.
3. **False.** Each `y[i]` is written by exactly one thread (the one that
   owns row `i`). `A` and `x` are only read. A race needs two threads
   accessing the same variable with at least one writing, so no lock is
   needed.
4. **False.** It still gives the right answer: only the thread whose
   number equals `flag` can add. It becomes very **slow**, because the
   thread whose turn it is may not be running, and the others spin
   uselessly until it gets a core again.
5. **False.** Writing needs the lock **alone**: no other writer and no
   reader (L04 slide 41). A read-write lock protects the whole list; it does
   not know which node a thread will change.
6. **True.** Each thread holds only one or two node locks at a time. Two
   `Member` calls in different parts of the list do not block each other.
   (They do block each other when one wants a node the other holds: per-node
   mutexes do not allow two readers on the **same** node.)
7. **False.** The operating system decides when each thread runs, so the
   lines can come out in any order, and the order changes between runs
   (L04 slide 8). The *returned values* collected by `main` are always in
   index order, because `main` joins thread 0, then 1, and so on.
8. **False.** A test only shows that no race caused a *visible wrong result*
   in those runs. A race may need a rare timing, or may not change the
   result on this machine. A race detector such as ThreadSanitizer
   (`make tsan`) checks every access, and the interleaving explorer checks
   every order of steps for small models.

## B. Short answer

9. (a) **1, 2 or 3.** The thread was created when `i` was 1, so it reads
   `i` at some moment after that: `i` is then 1, 2, or 3 (3 is the value
   after the loop ends).
   (b) `0 0 2` is **impossible**: only the thread created when `i` was 0 can
   ever see 0, so two zeros cannot appear. `3 3 3` is **possible**: all
   three threads may start only after the loop has finished. (The explorer
   lists 14 possible outputs. They never decrease, because `i` only grows.)
   (c) Give each thread its own value: `pthread_create(&t[i], NULL, show,
   (void *)(intptr_t)i)` and print `(long)(intptr_t)arg`; or use an array
   `int ids[3]` with `ids[i] = i` and pass `&ids[i]`.
10. All threads get the address of the **same** struct `a`. By the time a
    thread reads `a.id`, `main` may have changed it, so several threads see
    the same id (the same bug as `&i`). Fix: an array of structs,
    `struct arg a[P];`, set `a[t].id = t`, and pass `&a[t]`. Each struct must
    stay alive until the thread has finished with it (until the join).
11. 1. T0 loads `sum` (say 1.0). 2. T1 loads `sum` (1.0). 3. T0 adds its
    term and stores the result. 4. T1 adds its term to the **old** 1.0 and
    stores. T1's store overwrites T0's, so T0's term is lost. That is why the
    two-thread answers differ from run to run (L04 slide 20).
12. (a) t0: rows 0–1 (`[0,2)`), t1: rows 2–4 (`[2,5)`), t2: rows 5–6
    (`[5,7)`), t3: rows 7–9 (`[7,10)`). Sizes 2, 3, 2, 3: never more than one
    apart, and every row is covered exactly once.
    (b) `m/p = 10/4 = 2` in integer division, so the threads get rows 0–1,
    2–3, 4–5, 6–7, and **rows 8 and 9 are never computed**. The lecture's
    rule only works when `p` divides `m`.
13. Any three: (1) a waiting thread sleeps instead of keeping a core 100%
    busy; (2) no fixed order: whoever arrives first goes first, instead of
    thread 2 waiting for 0 and 1 even if it finished first; (3) it keeps
    working well with more threads than cores, while busy-waiting collapses
    when the thread whose turn it is is not running; (4) it needs no care
    about the compiler keeping the flag in a register. Busy-waiting is
    acceptable when every thread has its own core (threads ≤ cores) and the
    wait is very short, because then putting a thread to sleep and waking it
    up again would cost more than spinning for a moment.
14. Up to 8 threads every thread has its own core, so the thread whose turn
    it is is always running and hands the turn on quickly. With more than 8
    threads, some threads are paused by the operating system. Often the
    thread whose turn it is is one of the paused ones. The running threads
    then spin until their time slice ends, and the next turn can only happen
    when the right thread is scheduled again. The strict order turns this
    into many long delays. With a mutex, waiting threads sleep and whoever is
    ready takes the lock next, so paused threads do not hold anyone up.
15. 1. A reads `next2` (7). 2. A sets `node3->next = 7`. 3. B reads `next2`
    (7). 4. A sets `next2 = 3` (list is `2 -> 3 -> 7`). 5. B sets
    `node4->next = 7`. 6. B sets `next2 = 4`. Now the list is `2 -> 4 -> 7`:
    B overwrote A's link, and node 3 is lost. (The explorer also finds an
    order that gives `2 -> 4 -> 3 -> 7`: nothing is lost, but the list is no
    longer sorted.)
16. Part A locks once per increment: 8 × 100,000 = **800,000** times. Part B
    locks once per thread: **8** times. In Part A all 8 threads want the
    same lock almost all the time (high contention). Most of their time goes
    into waiting, and every hand-over of the lock between cores is
    expensive. In Part B each thread counts in a private local variable with
    no lock at all, and the lock is used only 8 times in total.
17. `main` returns as soon as it has created the threads, and returning from
    `main` ends the whole process, killing threads that have not finished.
    So some or all threads may never print, and the program cannot collect
    their return values at all. The output changes between runs.
18. The program **hangs**. The first thread to lock keeps the mutex
    forever. Every other thread blocks in `pthread_mutex_lock` and never gets
    in. The first thread itself also blocks on its next
    `pthread_mutex_lock`, because it already holds the mutex (for a default
    Linux mutex, locking it again yourself waits forever). This is a
    deadlock: nobody can ever continue.

## C. Design and proof

19. (a) **Read-write lock.** Almost every operation is a lookup (a read),
    and readers can hold the lock together, so lookups run in parallel. The
    rare writes lock it alone, briefly.
    (b) **One mutex for the list.** Nearly every operation is a write, so a
    read-write lock gives no extra parallelism, and per-node locking adds a
    lock and unlock at every step for little gain. One mutex is simple and
    has the lowest cost per operation (L04 slide 37).
    (c) **One mutex per node.** Writes are common, so a read-write lock would
    often be held by a writer and block everyone. With per-node locks,
    threads in different regions of the list do not block each other, and
    the extra memory per node is acceptable here.
20. (a) Any other thread that wants to reach `c` must come from `pred`, and
    hand-over-hand means it must lock `pred` first. We hold `pred`'s lock
    while we unlink `c`, so no thread can be on its way from `pred` to `c`.
    A thread already past `c` cannot come back. A thread holding `c` itself
    is impossible, because we hold `c`'s lock. So once `c` is unlinked, nobody
    can reach it, and freeing it is safe.
    (b) Every thread takes node locks in list order, from the front to the
    back, and only waits for a node further back than every lock it holds.
    For a deadlock, thread A would have to wait for a node held by B while B
    waits for a node held by A. That would need one of them to wait for a
    node in **front** of one it holds, which never happens.
21. In the 80/10/10 mix, one call in five is a write. A writer must wait for
    all readers to leave and then holds the lock alone, and while a writer
    waits or works, new readers are blocked. With several threads there is
    almost always a writer waiting or working, so the threads take turns
    almost as with one mutex. On top of that, a read-write lock costs a bit
    more than a mutex to lock and unlock. One mutex per node is slower than
    both because every step along the list locks and unlocks a mutex: an
    operation that walks 500 nodes does about 1000 lock operations instead of
    2.

## D. Code

22. One possible answer (exercise 5 tests it):
    ```c
    struct sum_arg { const long *a; long first, last; };
    static long total;
    static pthread_mutex_t total_lock = PTHREAD_MUTEX_INITIALIZER;

    static void *sum_worker(void *p) {
        struct sum_arg *arg = p;
        long mine = 0;                                     // private partial sum
        for (long i = arg->first; i < arg->last; i++) mine += arg->a[i];
        pthread_mutex_lock(&total_lock);
        total += mine;                                     // one short critical section
        pthread_mutex_unlock(&total_lock);
        return NULL;
    }

    long threaded_sum(const long *a, long n, int p) {
        pthread_t tid[p];
        struct sum_arg args[p];                            // one struct per thread
        total = 0;
        for (int t = 0; t < p; t++) {
            args[t] = (struct sum_arg){a, (long)t * n / p, (long)(t + 1) * n / p};
            pthread_create(&tid[t], NULL, sum_worker, &args[t]);
        }
        for (int t = 0; t < p; t++) pthread_join(tid[t], NULL);
        return total;                                      // safe: all threads joined
    }
    ```
    Marks for: struct per thread (1), correct blocks (1), local sum (1),
    lock/unlock around one addition (1), create then join all (1), returning
    only after the joins (1).
23. ```c
    int Member(int v) {
        pthread_rwlock_rdlock(&list_rwlock);      // reading: other readers may be inside too
        struct node *c = head.next;
        while (c != NULL && c->value < v) c = c->next;
        int found = (c != NULL && c->value == v);
        pthread_rwlock_unlock(&list_rwlock);
        return found;
    }
    ```
    `Insert` must call `pthread_rwlock_wrlock`, because it changes `next`
    pointers. If it only took the read lock, a reader could follow a
    half-updated link, or two inserters could overwrite each other's link
    (question 15).
