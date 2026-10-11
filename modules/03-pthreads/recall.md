# 03 · Recall sheet

Cover the right-hand column. Say each answer out loud, or write it on a
blank page. If you hesitate, re-read that part of the [README](README.md).

| Question | What you should be able to say or write |
|---|---|
| What is shared between threads, and what is private? | Global variables and memory from `malloc` are shared. Local variables are private, because each thread has its own stack. |
| What is the fork/join pattern? | The main thread starts (forks) worker threads, each does a part of the work, and the main thread waits (joins) for all of them before it continues. |
| Write the `pthread_create` call and say what each argument is. | `pthread_create(&tid, NULL, fn, arg)`: where to store the handle, default options, the function `void *fn(void *)` to run, and the one value passed to it. |
| What does `pthread_join(tid, &ret)` do? | It waits until that thread has finished and stores its return value in `ret`. Everything the thread wrote is safe to read afterwards. |
| How do you compile a Pthreads program? | `gcc -pthread -o prog prog.c` (the slides write `-lpthread`). |
| How do you pass a small integer to a thread? | Put it in the pointer value: `(void *)(intptr_t)i`, and take it out with `(long)(intptr_t)arg`. |
| What is wrong with passing `&i` of the loop counter? | Every thread gets the address of the same variable, which `main` keeps changing. Threads read whatever `i` is when they start, so they can see the same or later values. |
| How do you pass several values, and get results back? | Give each thread a pointer to its own struct. The thread writes its results into "out" fields, and `main` reads them after the join. |
| Write the block rule for thread t of p, n items. | `first = t*n/p`, `last = (t+1)*n/p`; the thread handles `first .. last-1`. It works even when p does not divide n. |
| Why does the matrix-vector product need no lock? | Each thread writes only its own entries of `y`; `A` and `x` are only read. A race needs two threads accessing the same variable with at least one writing. |
| Why does the two-thread pi estimate change between runs? | `sum += term` is load, add, store. Two threads can load the same old value, and one thread's addition is lost. |
| How does the busy-wait global sum work? | `while (flag != my_id) ;  sum += my_part;  flag = (flag + 1) % p;`. Only the thread whose turn it is can add. |
| What are the costs of busy-waiting? | It keeps a core busy doing nothing, forces a fixed order, and becomes very slow with more threads than cores (the thread whose turn it is may not be running). |
| Write the mutex calls. | `pthread_mutex_init(&m, NULL)`, `pthread_mutex_lock(&m)`, `pthread_mutex_unlock(&m)`, `pthread_mutex_destroy(&m)`. |
| Why accumulate locally and lock once? | Locking around every addition makes the threads queue for the lock (high contention). A local sum needs no lock, and the lock is then used only once per thread. |
| What does a read-write lock allow? | Many readers together, or one writer alone, never both. `rdlock` for Member, `wrlock` for Insert and Delete, `unlock` for both. |
| Three ways to lock a shared linked list, with one cost each. | One mutex for the list: only one thread at a time. One mutex per node (hand-over-hand): a lock and unlock at every step, plus memory per node. Read-write lock: no help when writes are common. |
| Explain hand-over-hand locking. | Lock the next node before unlocking the current one. You always hold the node you stand on, so nobody can delete it under you. |
| Which list lock wins at 99.9% Member? At 80/10/10? | 99.9% Member: the read-write lock, because readers run together. 80/10/10: the read-write lock is no better than one mutex; per-node locking was slowest in the lecture. |
| What happens if a thread forgets to unlock? | Every other thread (and the same thread, on its next lock) waits forever: the program hangs. |

**From memory:** write `threaded_sum` (exercise 5): struct per thread,
local partial sums, one locked addition, create and join. Then write
hand-over-hand `Member` for the per-node list.
