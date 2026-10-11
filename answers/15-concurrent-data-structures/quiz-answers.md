# 15 · Quiz answers: concurrent data structures

These answers are about the length a marker expects for the marks shown.
"Why" is always part of the answer.

## A. True / false

1. **False.** The CAS-loop counter is lock-free and loops. Lock-freedom is a
   promise about progress: every failed round means some *other* operation
   succeeded, so the system as a whole keeps moving.
2. **True.** Wait-free means *every* call finishes in a bounded number of its
   own steps; then certainly *some* call does (L12 slide 14).
3. **False.** A spin lock is still a lock. If the thread holding it is paused,
   every other thread spins forever without finishing: that is **blocking**.
   What matters is the progress guarantee, not whether a `pthread_mutex_t`
   appears.
4. **False.** Each step down the list costs a lock and an unlock, and threads
   cannot overtake each other. In Lab 05's benchmark (short operations, 50%
   lookups) the coarse list is usually faster. Fine-grained helps when the
   work per node is large or threads use different parts of the structure.
5. **False.** The node is already in the queue after the *first* CAS (logical
   enqueue). If the second CAS fails, another thread has already moved
   `tail` forward for us (it helped), so the failure can be ignored (slide 74).
6. **True.** Enqueuers use `enqLock` and the tail end; dequeuers use
   `deqLock` and the head end. They share only the atomic `permits` counter.
   (They interact only around empty and full, through the condition
   variables.)
7. **False.** The CAS writes `old + 1` only if the counter still equals
   `old`. If another thread changed it in between, the CAS fails and the
   thread retries with the new value, so no increment is overwritten.

## B. Short answer

8. (a) **Blocking:** if the thread holding `m` is paused, no other thread can
   finish an increment.
   (b) **Lock-free:** a CAS fails only because another thread's CAS
   succeeded, so some increment always finishes; one thread may retry forever,
   so not wait-free.
   (c) **Wait-free:** a single atomic instruction, finished in one step
   whatever the others do.
   (d) **Obstruction-free** (the intended answer): the increment is
   guaranteed to happen only if no other thread interferes; under contention
   it can silently skip the increment. (You may argue the *method* is
   wait-free because it always returns within a few steps; then say clearly
   that it is not a correct counter.)
   **Real-time:** choose **wait-free** (c). Only wait-freedom bounds the steps
   of *every* call. Lock-free code can make one particular thread retry an
   unbounded number of times, and blocking code can wait for a paused thread
   for as long as it is paused.
9. 1. A (insert 20) holds 5 and reads `curr = 12`.
   2. A unlocks 5 and now holds nothing.
   3. B (delete 12) locks 5 and 12, sets `5->next = 30`, unlocks, frees 12.
   4. A locks 12 (a removed, freed node); 12 < 20 so it moves on: prev = 12,
      curr = 30.
   5. A sets `12->next = new node 20`. Node 12 is not in the list, so 20 is
      never reachable: the insert is lost (and freed memory was written).
   With hand-over-hand, A would lock 12 *before* unlocking 5, and B could
   not have locked 5 until A moved past 12.
10. **Lock-free:** a CAS fails only if the counter changed after this thread
    read it, and it only changes when another thread's CAS succeeded. So
    every failure is someone else's success, and the system always makes
    progress. **Not wait-free:** nothing bounds how often *this* thread
    loses. In every round another thread can slip in between its read and its
    CAS. **Starvation:** in theory yes, a thread can retry forever under
    extreme contention. In practice it usually gets through, but its delay is
    unbounded. Backoff reduces this but does not remove it.
11. Fetch-and-add is one hardware instruction that always succeeds. The cache
    line goes to each core once per increment. A CAS loop needs a read and a
    CAS, and under contention the CAS often fails. Each failed attempt has
    pulled the cache line over for nothing, and must be repeated. So the CAS
    loop does far more work per successful increment.
12. 1. Consumers A and B find the queue empty and sleep on `notEmpty`.
    2. Producer C adds an item. The queue went from empty to 1, so C signals:
       A wakes (but has not yet re-acquired the lock).
    3. Producer D adds an item. 1 → 2 is not "empty to non-empty", so no
       signal.
    4. A gets the lock, takes one item, leaves.
    5. One item is still in the queue, but B is asleep and no producer will
       signal again: **B sleeps forever** (a lost wake-up, slides 56–60).
    **Broadcast** wakes *all* sleepers in step 2. A and B both wake up, and
    each re-checks the queue in its `while` loop, so both items are taken.
13. Every enqueue *and* every dequeue does an atomic update of the same
    `permits` variable. The cache line holding it must move to each core in
    turn, so all calls fight over one location, just like the shared counter
    (slide 62). The split: enqueuers only decrement `enqSidePermits` (under
    `enqLock`), dequeuers only increment `deqSidePermits` (under `deqLock`),
    so normally the two sides touch different variables. Only when an
    enqueuer runs out of permits does it take `deqLock` and move the
    dequeuers' permits over (slides 63–64). The two sides now synchronize
    occasionally instead of on every call. **New danger:** the enqueuer holds
    `enqLock` and then takes `deqLock`, so holding two locks at once can
    deadlock unless every thread takes them in the same fixed order.
14. (a) log₂ 16 = **4 levels**. (b) **Once**: all 16 increments are combined
    into one sum that the final winner adds at the root. (c) *Advantage:*
    increments proceed in parallel up the tree, with O(log p) latency instead
    of 16 threads fighting over one location; speedup about p / log p
    (slide 10). *Disadvantages* (slides 10–11): a lone thread still climbs
    4 levels, so it is slow with little contention; p must be fixed in
    advance; it needs O(p) memory; parallelism shrinks near the root; threads
    beyond p must wait.
15. 1. B reads `last = tail`, which still points at the node before A's new
       node, and `next = last->next`, which is A's node (not NULL).
    2. B checks that `tail` has not changed since it read it.
    3. Because `next` is not NULL, the tail is lagging. B does
       `CAS(tail, last, next)`, moving `tail` to A's node.
    4. B starts its loop again: now `tail->next` is NULL, so B does its own
       logical CAS and then its physical CAS.
    5. When A wakes up, its `CAS(tail, last, node)` fails because `tail` has
       already moved; A ignores that and returns.
    B finishes A's leftover step for it, which is why it is called
    **helping**. Without it, B would have to wait for A to move `tail`, and if
    A were paused forever, B would wait forever: blocking. With helping, a
    paused thread cannot stop the others, and every CAS failure means another
    thread made progress, so the queue is lock-free.
16. The sentinel means the list is never truly empty, so `head` and `tail`
    always point to a real node and enqueue/dequeue need no special case for
    "first item" or "last item" that would make them touch the same
    pointer. Making the first real node the new sentinel means a dequeue only
    changes `head` (and reads that node's value). It never changes a `next`
    pointer that an enqueuer might be using at the same moment, so the two
    ends stay independent (slide 38).
17. A dequeuer checks "is it empty?" and goes to sleep while holding
    `deqLock` (`pthread_cond_wait` releases it only as it starts sleeping).
    If the enqueuer could broadcast without `deqLock`, the broadcast could
    happen between the dequeuer's check and its sleep. The dequeuer would
    miss it and sleep forever. Taking `deqLock` makes the broadcast wait
    until the dequeuer is really asleep. **No deadlock:** the enqueuer
    releases `enqLock` *before* it takes `deqLock`, and the dequeuer does the
    same the other way round. Nobody ever holds both locks, so no two
    threads can each hold one lock and wait for the other.

## C. Is it correct?

18. **Give up (8d): outputs 1 or 2.** Both threads read 0; T0's CAS succeeds
    (count = 1); T1's CAS expects 0, sees 1, fails, and T1 gives up. The
    third thread prints **1**. If the CASes do not collide, it prints 2. The
    explorer confirms `1` is possible (`--why "1"` shows exactly this run).
    **CAS loop (8b): only 2.** T1's failed CAS makes it read 1 and retry with
    CAS(1 → 2), which succeeds. `--outputs` on the finished m1 model lists
    only `2`.
19. **Not linearizable.** `enq(1)` returned before `enq(2)` was called, so 1
    must be in the queue before 2, in every explanation. For `deq()` to
    return 2, 2 would have to be at the front, so 1 would have to be removed
    first, but no operation removes 1. Any instant we pick for `deq()` gives
    either an empty queue (before both enqueues) or a queue starting with 1.
    **If the two enqueues overlapped**, we could place `enq(2)`'s instant
    before `enq(1)`'s. Then the queue is 2, 1 and `deq() -> 2` is allowed:
    linearizable.
20. Stack: top → A → B → C.
    1. Thread 1 starts pop: reads `top = A` and `A->next = B`, and is paused.
    2. Thread 2 pops A, then pops B and frees it.
    3. Thread 2 pushes a new node, and malloc happens to give it A's old
       address. Now top → A → C.
    4. Thread 1 resumes: `CAS(top, A, B)` succeeds, because top is A again.
       Now top → B, a freed node: the stack is corrupted.
    The CAS compares only addresses, so it cannot see that A was removed and
    came back ("A, then B, then A again"). Our exercises never free or reuse a
    node while the queue is in use, so an address can never come back, and no
    thread can read memory that was freed under it. Real code uses version
    counters, hazard pointers or epoch-based reclamation instead.

## D. Code

21. ```c
    long get_and_increment(shared_long *c) {
        long old = LOAD(*c);
        while (!CAS(*c, old, old + 1))
            old = LOAD(*c);
        return old;
    }
    ```
22. ```
    enq(v):
        node = new node(v); node.next = NULL
        loop:
            last = tail; next = last.next
            if last == tail:                         // still consistent?
                if next == NULL:
                    if CAS(last.next, NULL, node):   // LOGICAL enqueue
                        CAS(tail, last, node)        // PHYSICAL enqueue (failure is fine)
                        return
                else:
                    CAS(tail, last, next)            // help the lagging tail
    ```
23. ```
    enq(v):
        node = new node(v)
        lock(enqLock)
        while permits == 0: wait(notFull, enqLock)
        tail.next = node; tail = node
        old = fetch_and_add(permits, -1)
        unlock(enqLock)
        if old == capacity:                  // the queue was empty
            lock(deqLock); broadcast(notEmpty); unlock(deqLock)
    ```
