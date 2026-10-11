// ra.h - shared types for the Ricart-Agrawala exercises (2 and 3). Do not change.
#ifndef RA_H
#define RA_H

#define RA_MAXN 8

enum { RA_RELEASED = 0, RA_WANTED = 1, RA_HELD = 2 };   // a node's state
enum { MSG_REQUEST = 1, MSG_REPLY = 2 };                 // message types

typedef struct {
    int type;      // MSG_REQUEST or MSG_REPLY
    int from, to;  // node ids
    int ts;        // the sender's Lamport clock when it sent the message
} ra_msg;

typedef struct {
    int id, n;                 // my id (0 .. n-1) and the number of nodes
    int clock;                 // my Lamport clock
    int state;                 // RA_RELEASED, RA_WANTED or RA_HELD
    int my_ts;                 // timestamp of my current request
    int replies;               // replies received for my current request
    int deferred[RA_MAXN];     // deferred[j] == 1: I still owe node j a reply
} ra_node;

// Provided by whoever runs the nodes (the test, or the demo main): delivers
// the message LATER, in any order, to node m->to.
void net_send(const ra_msg *m);

#endif
