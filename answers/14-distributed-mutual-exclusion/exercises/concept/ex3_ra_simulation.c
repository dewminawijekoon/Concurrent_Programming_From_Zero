// Exercise 3 (core) - Ricart-Agrawala with N simulated nodes   [EXTRA; README section 3.4]
//
// PREDICT FIRST: with 5 nodes, how many messages does one entry into the
//                critical section cost? Does that change when nobody else
//                wants to enter at the same time?
//
// The nodes live in one program and talk only through net_send(), which
// delivers each message LATER and in a RANDOM order (like a real network,
// without any promise about order). The test runs thousands of random
// schedules and checks:
//   - never two nodes in state HELD at the same time (mutual exclusion),
//   - every request is eventually granted (no deadlock, no starvation),
//   - every entry costs exactly 2(N-1) messages.
// Write the three event handlers. Use ra_should_defer() from exercise 2
// (this file includes it, so finish exercise 2 first).
//
// Lamport clock rules: tick before sending a REQUEST (the request carries
// that timestamp); on every received message, clock = max(clock, m->ts) + 1.
//
// Test:  make test M=14 T=ex3_ra_simulation
// Run:   make run  M=14 EX=ex3_ra_simulation
#include <stdio.h>
#include <stdlib.h>
#include "ra.h"
#define RA_DECISION_NO_MAIN
#include "ex2_ra_decision.c"      // ra_earlier, ra_should_defer

static void send_to(ra_node *me, int type, int to, int ts) {
    ra_msg m = {type, me->id, to, ts};
    net_send(&m);
}

// The node wants to enter: ask everybody else.
void ra_request_cs(ra_node *me) {
    me->state = RA_WANTED;
    me->clock++;                          // the request is an event: tick
    me->my_ts = me->clock;                // everyone compares against this stamp
    me->replies = 0;
    for (int j = 0; j < me->n; j++)
        if (j != me->id) send_to(me, MSG_REQUEST, j, me->my_ts);
    if (me->n == 1) me->state = RA_HELD;  // nobody to ask
}

// A message arrived.
void ra_on_message(ra_node *me, const ra_msg *m) {
    me->clock = (me->clock > m->ts ? me->clock : m->ts) + 1;     // Lamport receive rule
    if (m->type == MSG_REQUEST) {
        if (ra_should_defer(me->state, me->my_ts, me->id, m->ts, m->from))
            me->deferred[m->from] = 1;    // answer when I leave
        else
            send_to(me, MSG_REPLY, m->from, me->clock);
    } else if (m->type == MSG_REPLY) {
        me->replies++;
        if (me->state == RA_WANTED && me->replies == me->n - 1)
            me->state = RA_HELD;          // everyone said yes: enter
    }
}

// The node leaves the critical section: answer everyone it kept waiting.
void ra_release_cs(ra_node *me) {
    me->state = RA_RELEASED;
    for (int j = 0; j < me->n; j++)
        if (me->deferred[j]) {
            me->deferred[j] = 0;
            send_to(me, MSG_REPLY, j, me->clock);
        }
}

#ifndef TESTING
// ---------------------------------------------------------------- demo: 3 nodes, a small random run
static ra_msg pool[1000];
static int npool = 0;
void net_send(const ra_msg *m) { pool[npool++] = *m; }

int main(void) {
    ra_node node[3] = {{0}};
    for (int i = 0; i < 3; i++) { node[i].id = i; node[i].n = 3; }
    srand(1);
    ra_request_cs(&node[0]);
    ra_request_cs(&node[2]);
    printf("nodes 0 and 2 both request; %d messages in the network\n", npool);
    int entries = 0;
    while (entries < 2) {
        if (npool > 0) {
            int k = rand() % npool;
            ra_msg m = pool[k];
            pool[k] = pool[--npool];
            printf("deliver %-7s %d -> %d (ts %d)\n", m.type == MSG_REQUEST ? "REQUEST" : "REPLY", m.from, m.to, m.ts);
            ra_on_message(&node[m.to], &m);
        }
        for (int i = 0; i < 3; i++)
            if (node[i].state == RA_HELD) {
                printf("  node %d ENTERS the critical section (its request had ts %d)\n", i, node[i].my_ts);
                ra_release_cs(&node[i]);
                entries++;
            }
        if (npool == 0 && entries < 2) { printf("stuck: no messages left but not everyone entered\n"); break; }
    }
    return 0;
}
#endif
