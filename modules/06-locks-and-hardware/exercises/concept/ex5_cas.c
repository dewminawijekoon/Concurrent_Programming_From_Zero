// Exercise 5 (core) - building other atomic operations from compare-and-swap
//                     [L08-TLP slide 31; README section 5]
//
// CAS(x, expected, new) does this in ONE step:
//     if x == expected: x = new, return true
//     else:             leave x alone, return false
//
// With CAS you can build almost any atomic update with the same pattern:
//
//     loop:
//         old = LOAD(x)                 // 1. read the current value
//         new = something computed from old    // 2. work out the new value
//         if CAS(x, old, new): done     // 3. write it ONLY IF nobody changed x
//                                       //    since step 1; otherwise try again
//
// If another thread changed x between steps 1 and 3, the CAS fails and you
// simply start again with the fresh value. No update is ever lost.
//
// (a) cas_fetch_increment(x): add 1 to x and return the OLD value
//     (this is the hardware's fetch-and-increment, built from CAS).
// (b) cas_max(x, v): make x = max(x, v) atomically. This is the atomic
//     max() the Bakery slides ask about ("what if max() is not atomic?").
//
// PREDICT FIRST: 4 threads each call cas_fetch_increment 100000 times on
//                the same x (starting at 0). Which return values do you
//                expect to see, and how many times each?
//
// Do NOT use FETCH_ADD here; use LOAD and CAS only.
//
// Test:  make test M=06 T=ex5_cas
#include "atomics.h"

long cas_fetch_increment(shared_long *x) {
    // TODO: the read / compute / CAS loop above
    (void)x;
    return 0;
}

void cas_max(shared_long *x, long v) {
    // TODO: same loop; you can stop early if x is already >= v
    (void)x; (void)v;
}
