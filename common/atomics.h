/*
 * atomics.h — make real C behave like the lecture model.
 *
 * The lock algorithms in the lectures (Peterson, Filter, Bakery, ...) assume
 * that every read and write of a shared variable is atomic and that all
 * threads see those reads and writes in one single order ("sequential
 * consistency").
 *
 * Plain C variables do NOT give you that:
 *   1. The compiler may keep a variable in a register, so a loop like
 *      `while (flag[j]) {}` can spin forever on a stale copy.
 *   2. The CPU may reorder a store with a later load (x86 does exactly this
 *      through its store buffer), which breaks Peterson's lock.
 *
 * Wrapping shared variables in C11 atomics with the default (seq_cst)
 * ordering restores the lecture model. Use these helpers in the lock
 * exercises so you can concentrate on the algorithm.
 * See modules/05-mutual-exclusion/break_it/ for what goes wrong without them.
 */
#ifndef ATOMICS_H
#define ATOMICS_H

#include <stdatomic.h>
#include <stdbool.h>

/* A shared integer / boolean that behaves like the lecture's variables. */
typedef atomic_int shared_int;
typedef atomic_bool shared_bool;
typedef atomic_long shared_long;

/* Atomic read and write (sequentially consistent). */
#define LOAD(var) atomic_load(&(var))
#define STORE(var, value) atomic_store(&(var), (value))

/* Read-modify-write instructions provided by the hardware. */

/* getAndIncrement-style: returns the OLD value, adds `delta`. */
#define FETCH_ADD(var, delta) atomic_fetch_add(&(var), (delta))

/* TestAndSet: set to true, return the OLD value. */
#define TEST_AND_SET(var) atomic_exchange(&(var), true)

/* Exchange: write `value`, return the OLD value. */
#define EXCHANGE(var, value) atomic_exchange(&(var), (value))

/*
 * CompareAndSet(var, expected, new): if var == expected, set var = new and
 * return true; otherwise leave var unchanged and return false.
 * (Unlike the raw C11 call, `expected` is passed by value and not modified.)
 */
#define CAS(var, expected, newval)                                         \
    ({                                                                     \
        __typeof__(atomic_load(&(var))) cas_exp_ = (expected);             \
        atomic_compare_exchange_strong(&(var), &cas_exp_, (newval));       \
    })

/* Polite busy-wait hint: tells the CPU we are spinning. */
static inline void cpu_relax(void) {
#if defined(__x86_64__) || defined(__i386__)
    __builtin_ia32_pause();
#elif defined(__aarch64__)
    __asm__ __volatile__("yield");
#endif
}

#endif /* ATOMICS_H */
