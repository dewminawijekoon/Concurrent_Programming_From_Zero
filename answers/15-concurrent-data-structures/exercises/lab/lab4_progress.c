// Lab track 4 - progress conditions: blocking, obstruction-free, lock-free, wait-free
//                                              [Lab 05, Ex 4; L12 slides 12-16]
//
// PREDICT FIRST: a thread is paused by the operating system for one second
//                in the MIDDLE of an increment. For each version below: can
//                the other threads still finish their increments meanwhile?
//
// The four progress conditions, from strongest to weakest (L12 slide 13):
//   WAIT_FREE         every call finishes within a bounded number of ITS OWN
//                     steps, whatever the other threads do.
//   LOCK_FREE         in any run, SOME call finishes after a bounded number of
//                     steps; one unlucky thread may retry forever.
//   OBSTRUCTION_FREE  a call is guaranteed to finish only if it runs ALONE
//                     for long enough (no interference from other threads).
//   BLOCKING          a thread that is paused (while holding a lock, say)
//                     can stop the others from finishing.
//
// Part 1: classify each snippet by returning one of the four values.
//         (These are our own versions of the kinds of code in Lab 05 Ex 4.)
//   'A'  lock(&m); counter++; unlock(&m);
//   'B'  do { old = LOAD(counter); } while (!CAS(counter, old, old + 1));
//   'C'  FETCH_ADD(counter, 1);
//   'D'  old = LOAD(counter); if (!CAS(counter, old, old + 1)) return;   // give up
//   'E'  while (TEST_AND_SET(busy)) {}  counter++;  STORE(busy, false);
//   'F'  like B, but after each failed CAS wait a random, growing time (backoff)
//   'G'  every thread has its own slot: STORE(slot[me], LOAD(slot[me]) + 1);
//        (only thread `me` ever writes slot[me]; the total is the sum of all slots)
//
// Part 2: write snippet D as a real function: try_increment_once(c) makes ONE
//         CAS attempt to add 1 to *c, returns 1 if it worked and 0 if not.
//
// Test:  make test M=15 T=lab4_progress
#include <stdio.h>

#include "atomics.h"

enum progress { UNANSWERED = 0, BLOCKING, OBSTRUCTION_FREE, LOCK_FREE, WAIT_FREE };

enum progress classify(char snippet) {
    switch (snippet) {
    case 'A': return BLOCKING;          // a paused lock holder stops everybody
    case 'B': return LOCK_FREE;         // a CAS fails only because another one succeeded
    case 'C': return WAIT_FREE;         // one instruction, always finishes
    case 'D': return OBSTRUCTION_FREE;  // the increment is sure to happen only when alone
    case 'E': return BLOCKING;          // a spin lock is still a lock
    case 'F': return LOCK_FREE;         // backoff makes it faster, not stronger
    case 'G': return WAIT_FREE;         // nobody else writes my slot: a fixed number of steps
    default: return UNANSWERED;
    }
}

int try_increment_once(shared_long *c) {
    long old = LOAD(*c);
    return CAS(*c, old, old + 1) ? 1 : 0;
}

#ifndef TESTING
int main(void) {
    const char *names[] = {"(no answer yet)", "blocking", "obstruction-free", "lock-free",
                           "wait-free"};
    for (char s = 'A'; s <= 'G'; s++) printf("snippet %c: %s\n", s, names[classify(s)]);
    return 0;
}
#endif
