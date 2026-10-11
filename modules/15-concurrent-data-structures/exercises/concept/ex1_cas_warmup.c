// Exercise 1 (warm-up) - three small CAS loops                [L12 slides 15-16]
//
// PREDICT FIRST: in atomic_max(), when may you stop WITHOUT doing a CAS at
//                all? And in the stack push, what must you do again after a
//                failed CAS?
//
// Every lock-free algorithm in this module uses the same recipe:
//     loop:
//         old = read the shared variable
//         compute the new value from old (in private, nobody sees it)
//         if CAS(shared, old, new) succeeds: done
//         else: somebody changed it in between, so start again
// The CAS fails ONLY because another thread's CAS succeeded, so the system
// as a whole always makes progress (lock-free). One unlucky thread can fail
// again and again, though, so it is not wait-free.
//
// Use LOAD and CAS from common/atomics.h. Write:
//   1. cas_fetch_add(x, d): add d to *x with a CAS loop; return the OLD value.
//   2. atomic_max(x, v):    make *x = max(*x, v) with a CAS loop. If *x is
//                           already >= v there is nothing to do.
//   3. tstack_push(s, n):   push node n on a lock-free stack (Treiber's stack):
//                           n->next = current top; CAS(top, that top, n).
//                           (Only push: pop has a nasty problem called ABA,
//                           see the README.)
//
// Test:  make test M=15 T=ex1_cas_warmup
#include <stddef.h>

#include "atomics.h"

typedef struct snode {
    long value;
    struct snode *next;
} snode;

typedef struct {
    _Atomic(snode *) top;    // NULL when empty
} tstack;

long cas_fetch_add(shared_long *x, long d) {
    // TODO
    (void)x; (void)d;
    return 0;
}

void atomic_max(shared_long *x, long v) {
    // TODO
    (void)x; (void)v;
}

void tstack_push(tstack *s, snode *n) {
    // TODO
    (void)s; (void)n;
}
