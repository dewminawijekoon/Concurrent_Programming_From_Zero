// Exercise 4 (from memory) - the Banker's safety check, no hints.
//
// Close the README and your ex3 solution. Write is_safe() from memory: the
// Need matrix, the work vector, the "find a process that can finish" loop,
// and the safe sequence. The ex3 safety tests apply. In the exam you may
// have to run this algorithm by hand on a table and write out each step.
//
// Test:  make test M=10 T=ex4_bankers_from_memory
#include "deadlock.h"

bool is_safe(int n, int m, const int available[MAXR], const int max[MAXP][MAXR],
             const int alloc[MAXP][MAXR], int sequence[MAXP]) {
    (void)n; (void)m; (void)available; (void)max; (void)alloc; (void)sequence;
    return false;
}
