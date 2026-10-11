// Exercise 1 (warm-up) - find a cycle in a resource-allocation graph
//                        [L11 slides 7, 10]
//
// With ONE instance of each resource, a set of processes is deadlocked
// exactly when the resource-allocation graph has a cycle. The arrows:
//   process -> resource   "the process is waiting for this resource"
//   resource -> process   "this resource is held by the process"
//
// Example: P holds R1 and wants R2; Q holds R2 and wants R1.
//   P -> R2 -> Q -> R1 -> P        a cycle: P and Q are deadlocked
//
// Use depth-first search (DFS): walk along arrows, keeping the path you are
// on. Give every node a colour:
//   WHITE = not visited yet, GREY = on the current path, BLACK = finished
//           (every path from it explored, and no cycle found there)
// If you follow an arrow to a GREY node, you have come back to your own
// path: that is a cycle. The cycle is the part of the path from that grey
// node to where you are now.
//
// PREDICT FIRST: A holds R and wants S; B holds S and wants T; C holds T
//                and wants R; D holds U and wants S; E holds V and wants U.
//                Draw the graph. Which processes are on the cycle? Which
//                others are stuck forever anyway? (The test uses this graph.)
//
// Implement find_cycle() (see deadlock.h for the exact contract).
//
// Test:  make test M=10 T=ex1_rag_cycle
// Run:   make run  M=10 EX=ex1_rag_cycle
#include <stdio.h>

#include "deadlock.h"

enum { WHITE, GREY, BLACK };

int find_cycle(int n, const bool edge[MAXN][MAXN], int cycle[MAXN]) {
    // TODO: run a DFS from every WHITE node. Keep the current path in an
    //       array (path[0..len-1]) so you can copy the cycle out of it.
    //       A recursive helper function is the easiest way.
    (void)n; (void)edge; (void)cycle;
    return 0;
}

#ifndef TESTING
int main(void) {
    // P holds R1 and wants R2; Q holds R2 and wants R1.
    const char *name[] = {"P", "Q", "R1", "R2"};
    bool e[MAXN][MAXN] = {{false}};
    e[2][0] = true;   // R1 -> P  (P holds R1)
    e[0][3] = true;   // P -> R2  (P wants R2)
    e[3][1] = true;   // R2 -> Q  (Q holds R2)
    e[1][2] = true;   // Q -> R1  (Q wants R1)
    int cyc[MAXN];
    int k = find_cycle(4, e, cyc);
    if (k == 0) { printf("no cycle found: no deadlock\n"); return 0; }
    printf("cycle: ");
    for (int i = 0; i < k; i++) printf("%s -> ", name[cyc[i]]);
    printf("%s   => deadlock\n", name[cyc[0]]);
    return 0;
}
#endif
