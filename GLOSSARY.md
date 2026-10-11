# Glossary

Plain-English meanings of the words used in this course, with a tiny example
for each. Lessons link here the first time they use a word.

### Atomic
A step that happens **all at once**. No other thread can see it half-done
or squeeze in between its parts.
*Example:* reading one `int` is atomic. `counter++` is **not**: it is three
steps (read, add one, write back), and another thread can run in between.

### Busy-waiting (spinning)
Waiting by checking a condition again and again in a loop, e.g.
`while (flag[j]) {}`. The thread keeps using the CPU while it waits.

### Critical section (CS)
The part of the code that uses shared data, where **only one thread at a
time** may be.
*Example:* the lines that read and update a shared bank balance.

### Deadlock
Every thread is **waiting for another one**, so nobody can ever move again.
*Example:* thread A holds lock 1 and waits for lock 2, while thread B holds
lock 2 and waits for lock 1.

### Fair scheduler
A scheduler that does not ignore a thread forever. If a thread is ready to
run all the time, it will get a turn eventually. We assume this when we talk
about livelock and starvation; otherwise any thread could "starve" just
because the scheduler never picks it.

### Interleaving (schedule)
One possible order in which the steps of different threads run.
*Example:* A1 B1 A2 B2 and A1 A2 B1 B2 are two interleavings of threads A
and B with two steps each.

### Livelock
Threads **keep running**, but nobody makes progress.
*Example:* two people in a corridor both step aside, then both step back,
forever.

### Liveness property
"Something good **eventually** happens."
*Example:* "every thread that wants the lock eventually gets it." You
disprove it by showing a run that goes on forever without the good thing.

### Lock (mutex)
An object with `lock()` and `unlock()` that lets only one thread at a time
pass `lock()` until that thread calls `unlock()`. "Mutex" is short for
*mutual exclusion*.

### Memory fence (barrier instruction)
A CPU instruction that says "finish all my earlier memory writes before
doing any later reads or writes". It stops the CPU from reordering memory
operations across it. (Not to be confused with a *thread barrier*, module
07.)

### Mutual exclusion
The rule that two threads are **never in their critical sections at the
same time**.

### Race condition (data race)
The result depends on the exact timing of threads.
*Example:* two threads each do `counter++` once; the result can be 1
instead of 2.

### Safety property
"Something bad **never** happens."
*Example:* "two threads are never in the critical section together." You
disprove it by showing one run that reaches the bad situation.

### Sequential consistency
The simple model the lectures use. All threads' steps happen in **one
single order**, and each thread's own steps stay in the order written.
Real CPUs only behave like this if you use atomics or fences (module 05,
section 6).

### Starvation
One thread **waits forever** while others keep getting in.
*Example:* a thread that never wins the lock because others always grab it
first.

### Thread
An independent sequence of steps inside a program. Threads of the same
program share memory: they can read and write the same variables.
