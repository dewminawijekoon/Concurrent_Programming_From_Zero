// Exercise 2 (core) - Ricart-Agrawala: reply now, or make them wait?   [EXTRA; README section 3.4]
//
// PREDICT FIRST: node 1 (request stamped 5) and node 2 (request stamped 5)
//                want the critical section at the same time. Each receives
//                the other's request. Who replies, and who waits?
//
// When a REQUEST stamped (req_ts, req_id) arrives, a node decides:
//   - state HELD (I am inside):                       DEFER (reply when I leave)
//   - state WANTED and MY request is earlier:          DEFER
//   - otherwise (RELEASED, or their request is earlier): REPLY NOW
// "Earlier" means: smaller timestamp; on equal timestamps, smaller node id.
//
// Test:  make test M=14 T=ex2_ra_decision
// Run:   make run  M=14 EX=ex2_ra_decision
#include <stdio.h>
#include "ra.h"

// Is request (ts_a, id_a) earlier than request (ts_b, id_b)?
int ra_earlier(int ts_a, int id_a, int ts_b, int id_b) {
    // TODO
    (void)ts_a; (void)id_a; (void)ts_b; (void)id_b;
    return 0;
}

// Return 1 to DEFER the reply, 0 to REPLY NOW.
int ra_should_defer(int my_state, int my_ts, int my_id, int req_ts, int req_id) {
    // TODO: the three cases above (use ra_earlier)
    (void)my_state; (void)my_ts; (void)my_id; (void)req_ts; (void)req_id;
    return -1;
}

#if !defined(TESTING) && !defined(RA_DECISION_NO_MAIN)
int main(void) {
    printf("node 1 (WANTED, ts 5) gets a request from node 2 (ts 5): %s\n",
           ra_should_defer(RA_WANTED, 5, 1, 5, 2) ? "defer" : "reply now");
    printf("node 2 (WANTED, ts 5) gets a request from node 1 (ts 5): %s\n",
           ra_should_defer(RA_WANTED, 5, 2, 5, 1) ? "defer" : "reply now");
    printf("node 0 (RELEASED) gets any request: %s\n",
           ra_should_defer(RA_RELEASED, 0, 0, 9, 3) ? "defer" : "reply now");
    return 0;
}
#endif
