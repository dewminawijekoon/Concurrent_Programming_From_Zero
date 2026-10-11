// Exercise 2 (core / challenge) - the Michael-Scott lock-free queue
//                                                      [L12 slides 65-76]
//
// PREDICT FIRST: enq() needs TWO CAS steps: link the node after the last
//                node, then move tail. Between them, another thread can run.
//                What does that thread see, and what should it do about it?
//
// Read ms_queue.h first (the node pool, and why nodes are never freed).
//
// enq(v), following L12 slide 71:
//     node = msq_new_node(q, v)
//     loop:
//         last = tail;  next = last->next
//         if last is still the tail:
//             if next == NULL:                       tail really is the last node
//                 if CAS(last->next, NULL, node):    LOGICAL enqueue: node is in
//                     CAS(tail, last, node)          PHYSICAL enqueue: may fail,
//                     return                         that is fine (someone helped)
//             else:                                  tail is lagging behind
//                 CAS(tail, last, next)              HELP: move it forward, retry
//
// deq(&v) (the notes give no code; this is the standard version [EXTRA]):
//     loop:
//         first = head;  last = tail;  next = first->next
//         if first is still the head:
//             if first == last:                      head and tail on the same node
//                 if next == NULL: return false      really empty
//                 CAS(tail, last, next)              tail lags: help, then retry
//             else:
//                 v = next->value                    read BEFORE the CAS: after it,
//                 if CAS(head, first, next):         next is the new sentinel
//                     return true                    (first is the old sentinel)
//
// Use LOAD, STORE and CAS from atomics.h on head, tail and next.
//
// Test:  make test M=15 T=ex2_ms_queue
#include "ms_queue.h"

void msq_enq(msq_t *q, long value) {
    // TODO: follow enq(v) above
    (void)q; (void)value;
}

bool msq_deq(msq_t *q, long *value) {
    // TODO: follow deq(&v) above
    (void)q; (void)value;
    return false;
}
