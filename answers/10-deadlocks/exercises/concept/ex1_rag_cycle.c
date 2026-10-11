// ANSWER - Exercise 1 (warm-up) - find a cycle in a resource-allocation graph
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

static int nn;
static const bool (*E)[MAXN];
static int colour[MAXN];
static int path[MAXN], len;
static int *out;

// Visit u. Returns the cycle length if a cycle was found below u, else 0.
static int dfs(int u) {
    colour[u] = GREY;               // u is now on the current path
    path[len++] = u;
    for (int v = 0; v < nn; v++) {
        if (!E[u][v]) continue;
        if (colour[v] == GREY) {    // back to a node on our own path: a cycle
            int start = 0;
            while (path[start] != v) start++;
            int k = 0;
            for (int i = start; i < len; i++) out[k++] = path[i];
            return k;
        }
        if (colour[v] == WHITE) {
            int k = dfs(v);
            if (k) return k;
        }
        // BLACK: already fully explored, no cycle through it
    }
    colour[u] = BLACK;              // finished: leave the path
    len--;
    return 0;
}

int find_cycle(int n, const bool edge[MAXN][MAXN], int cycle[MAXN]) {
    nn = n; E = edge; out = cycle; len = 0;
    for (int u = 0; u < n; u++) colour[u] = WHITE;
    for (int u = 0; u < n; u++)
        if (colour[u] == WHITE) {
            int k = dfs(u);
            if (k) return k;
        }
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
