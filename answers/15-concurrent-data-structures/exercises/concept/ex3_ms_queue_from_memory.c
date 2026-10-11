// Exercise 3 (from memory) - Michael-Scott queue enq and deq, no hints
//
// Close ex2 and the notes. Write msq_enq and msq_deq from a blank page. The
// types and the node pool are in ms_queue.h (not part of the algorithm).
// Remember the two CAS steps of enq (logical, then physical), what to do
// when the tail is lagging, and why a failed second CAS can be ignored.
// The same tests as ex2 apply. In the exam you may be asked for enq() as
// pseudocode and to explain "helping".
//
// Test:  make test M=15 T=ex3_ms_queue_from_memory
#include "ms_queue.h"

void msq_enq(msq_t *q, long value) {
    msq_node *node = msq_new_node(q, value);
    for (;;) {
        msq_node *last = LOAD(q->tail);
        msq_node *next = LOAD(last->next);
        if (last != LOAD(q->tail)) continue;          // tail moved while we read: retry
        if (next == NULL) {
            if (CAS(last->next, (msq_node *)NULL, node)) {   // logical enqueue
                CAS(q->tail, last, node);                    // physical enqueue (may fail)
                return;
            }
        } else {
            CAS(q->tail, last, next);                 // help the lagging enqueuer
        }
    }
}

bool msq_deq(msq_t *q, long *value) {
    for (;;) {
        msq_node *first = LOAD(q->head);
        msq_node *last = LOAD(q->tail);
        msq_node *next = LOAD(first->next);
        if (first != LOAD(q->head)) continue;
        if (first == last) {
            if (next == NULL) return false;           // empty
            CAS(q->tail, last, next);                 // tail lags behind: fix it first
        } else {
            long v = next->value;
            if (CAS(q->head, first, next)) {          // next becomes the new sentinel
                *value = v;
                return true;
            }
        }
    }
}
