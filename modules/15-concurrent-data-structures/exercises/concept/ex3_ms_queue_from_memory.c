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
    (void)q; (void)value;
}

bool msq_deq(msq_t *q, long *value) {
    (void)q; (void)value;
    return false;
}
