// Exercise 3 (from memory) - the Bakery lock, no hints.
//
// Close the slides and your ex2 solution. Write the whole lock from memory:
// the shared variables, init, the doorway, the waiting part, lock and
// unlock. The same tests as ex2 apply (including first-come-first-served).
// In the exam you may be asked to write it in pseudocode and to explain:
// why ties are broken by thread id, why it is first-come-first-served, and
// what happens to the labels over time.
//
// Test:  make test M=06 T=ex3_bakery_from_memory
#include "nlock.h"

void bakery_init(int n) {
    (void)n;
}

void bakery_doorway(int i) {
    (void)i;
}

void bakery_wait(int i) {
    (void)i;
}

void bakery_lock(int i) {
    (void)i;
}

void bakery_unlock(int i) {
    (void)i;
}
