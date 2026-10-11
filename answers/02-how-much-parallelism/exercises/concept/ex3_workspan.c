// Exercise 3 (core) - work, span and parallelism of a task graph   [L02 slides 18-26]
//
// PREDICT FIRST: a job has 4 tasks. A (2 s) must run first. Then B (3 s) and
//                C (1 s) can run at the same time. D (4 s) needs both B and C.
//                How long does it take on 1 processor? On 100 processors?
//
// A computation is a DAG (directed acyclic graph, L02 slide 19): each vertex
// is a piece of work (a "strand") that takes cost[v] time units, and an edge
// u -> v means "u must finish before v can start".
//
//   work         = total time on 1 processor = the sum of all costs (T1)
//   span         = time with unlimited processors = the cost of the most
//                  expensive path through the DAG (the "critical path", T-infinity)
//   parallelism  = work / span = the best speedup ANY number of processors can give
//
// The DAG is given as a list of edges. To keep this simple, the vertices are
// numbered so that every edge goes from a SMALLER number to a BIGGER one
// (u < v). So when you reach vertex v, all the vertices before it are done.
//
// Part A - workspan(nv, cost, ne, edges)
//   Return a struct dag_measures with work, span and parallelism.
//   Span idea: finish[v] = cost[v] + the largest finish[u] over all edges u -> v
//   (or just cost[v] if v has no incoming edge). Then span = the largest finish[v].
//   If some edge has u >= v (or a vertex number out of range), return all three as -1.
//
// Part B - tp_lower_bound(work, span, p)
//   No schedule on p processors can beat two limits (L02 slides 22, 25):
//     work law: Tp >= work / p     (p processors do at most p units per step)
//     span law: Tp >= span         (the critical path is done one step at a time)
//   Return the larger of the two.
//
// Test:  make test M=02 T=ex3_workspan
// Run:   make run  M=02 EX=ex3_workspan
#include <stdio.h>

#include "ex3_workspan.h"

struct dag_measures workspan(int nv, const double cost[], int ne, const int edges[][2]) {
    struct dag_measures bad = {-1, -1, -1};
    for (int e = 0; e < ne; e++) {
        int u = edges[e][0], v = edges[e][1];
        if (u < 0 || v >= nv || u >= v) return bad;    // numbering rule broken
    }
    double finish[DAG_MAX_VERTICES];
    double work = 0, span = 0;
    for (int v = 0; v < nv; v++) {
        double ready = 0;                               // when all of v's inputs are done
        for (int e = 0; e < ne; e++)
            if (edges[e][1] == v && finish[edges[e][0]] > ready) ready = finish[edges[e][0]];
        finish[v] = ready + cost[v];
        work += cost[v];
        if (finish[v] > span) span = finish[v];
    }
    struct dag_measures r = {work, span, span > 0 ? work / span : 0};
    return r;
}

double tp_lower_bound(double work, double span, int p) {
    double by_work = work / p;
    return by_work > span ? by_work : span;
}

#ifndef TESTING
int main(void) {
    // The PREDICT FIRST example: A=0 (2), B=1 (3), C=2 (1), D=3 (4)
    double cost[] = {2, 3, 1, 4};
    int edges[][2] = {{0, 1}, {0, 2}, {1, 3}, {2, 3}};
    struct dag_measures m = workspan(4, cost, 4, edges);
    printf("work %.1f, span %.1f, parallelism %.3f\n", m.work, m.span, m.parallelism);
    for (int p = 1; p <= 4; p++)
        printf("  on %d processor(s): at least %.2f time units\n", p,
               tp_lower_bound(m.work, m.span, p));
    return 0;
}
#endif
