// Exercise 3 (from memory) - Peterson's lock, no hints.
//
// Close the lecture slides and your ex1 solution. Write the whole lock from
// memory: shared variables, init, lock, unlock. The same tests as ex1 apply.
// In the exam you may be asked to write it in pseudocode and argue why it
// satisfies mutual exclusion, deadlock-freedom and starvation-freedom.
//
// Test:  make test M=05 T=ex3_peterson_from_memory
#include "atomics.h"
#include "ex1_peterson.h"

void peterson_init(void) {
}

void peterson_lock(int i) {
    (void)i;
}

void peterson_unlock(int i) {
    (void)i;
}
