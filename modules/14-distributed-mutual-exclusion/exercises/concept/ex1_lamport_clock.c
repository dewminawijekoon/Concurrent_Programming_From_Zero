// Exercise 1 (warm-up) - Lamport logical clocks          [EXTRA; L03 slides 28-29 (happens-before)]
//
// PREDICT FIRST: P1 receives a message stamped 7 while its own clock shows 2.
//                What does its clock show after the receive? Why can it not
//                stay at 3?
//
// A Lamport clock is just an integer counter in each process. Three rules:
//   1. local event:  clock = clock + 1
//   2. send:         clock = clock + 1, and the message carries this value
//   3. receive:      clock = max(clock, value in the message) + 1
// The event gets the clock value AFTER the update. Then: if event a
// happened-before event b, a's timestamp is smaller than b's.
// Ties (equal timestamps) are broken by process id, giving a total order.
//
// Test:  make test M=14 T=ex1_lamport_clock
// Run:   make run  M=14 EX=ex1_lamport_clock
#include <stdio.h>

// Rule 1. Returns the timestamp of the local event.
int lc_local(int *clock) {
    // TODO
    (void)clock;
    return 0;
}

// Rule 2. Returns the timestamp to put in the message.
int lc_send(int *clock) {
    // TODO
    (void)clock;
    return 0;
}

// Rule 3. Returns the timestamp of the receive event.
int lc_receive(int *clock, int msg_ts) {
    // TODO
    (void)clock; (void)msg_ts;
    return 0;
}

// Total order: is event (ts_a at process id_a) before event (ts_b at id_b)?
// Smaller timestamp first; on a tie, smaller process id first.
int lc_before(int ts_a, int id_a, int ts_b, int id_b) {
    // TODO
    (void)ts_a; (void)id_a; (void)ts_b; (void)id_b;
    return 0;
}

// A whole run. Events are listed in an order in which they really happened
// (every send comes before its receive). kind: 'L' local, 'S' send, 'R' receive.
// msg is a message number (0..63) linking a send to its receive.
typedef struct { int proc; char kind; int msg; } lc_event;

void lamport_run(const lc_event *ev, int n, int nprocs, int *out_ts) {
    int clock[16] = {0};                          // one clock per process (nprocs <= 16)
    int msg_ts[64] = {0};                         // timestamp carried by each message
    // TODO: go through the events in order; use the three functions above.
    //       Remember the timestamp of each send in msg_ts[msg], so the
    //       matching receive can use it.
    (void)ev; (void)nprocs; (void)clock; (void)msg_ts;
    for (int e = 0; e < n; e++) out_ts[e] = 0;
}

#ifndef TESTING
int main(void) {
    // The example from the README, section 3.1.
    lc_event ev[] = {
        {0, 'L', 0}, {1, 'L', 0}, {2, 'L', 0}, {0, 'S', 1}, {1, 'R', 1},
        {1, 'S', 2}, {2, 'L', 0}, {2, 'R', 2}, {2, 'S', 3}, {0, 'R', 3},
    };
    const char *name = "acfbdeghij";
    int ts[10];
    lamport_run(ev, 10, 3, ts);
    for (int e = 0; e < 10; e++) printf("event %c on P%d (%c): timestamp %d\n", name[e], ev[e].proc, ev[e].kind, ts[e]);
    return 0;
}
#endif
