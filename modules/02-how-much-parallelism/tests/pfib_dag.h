// pfib_dag.h - builds the computation DAG of P-FIB(n) (L02 slides 16-21).
//
// Every call P-FIB(n) with n >= 2 has three strands (pieces of code with no
// spawn or sync inside):
//   A: test n, then spawn P-FIB(n-1)
//   B: call P-FIB(n-2)
//   C: sync, then return x + y
// A call with n <= 1 is a single strand. Edges:
//   A -> first strand of P-FIB(n-1)     (spawn)
//   A -> B                              (continue in the same call)
//   B -> first strand of P-FIB(n-2)     (call)
//   B -> C, last strand of each child -> C   (sync / return)
// Vertices are numbered in creation order A, child 1, B, child 2, C, so every
// edge goes from a smaller number to a bigger one.
#ifndef PFIB_DAG_H
#define PFIB_DAG_H

#define PFIB_MAX_V 4096
#define PFIB_MAX_E 8192

struct pfib_dag {
    int nv, ne;
    int edges[PFIB_MAX_E][2];
};

static inline void pfib_edge(struct pfib_dag *g, int u, int v) {
    g->edges[g->ne][0] = u;
    g->edges[g->ne][1] = v;
    g->ne++;
}

static inline void pfib_build(int n, struct pfib_dag *g, int *first, int *last) {
    if (n <= 1) {
        *first = *last = g->nv++;
        return;
    }
    int a = g->nv++, f1, l1, f2, l2;
    pfib_build(n - 1, g, &f1, &l1);
    int b = g->nv++;
    pfib_build(n - 2, g, &f2, &l2);
    int c = g->nv++;
    pfib_edge(g, a, f1);
    pfib_edge(g, a, b);
    pfib_edge(g, b, f2);
    pfib_edge(g, b, c);
    pfib_edge(g, l1, c);
    pfib_edge(g, l2, c);
    *first = a;
    *last = c;
}

static inline void pfib_make(int n, struct pfib_dag *g) {
    int f, l;
    g->nv = g->ne = 0;
    pfib_build(n, g, &f, &l);
}

// Longest path (in vertices) of a DAG whose edges go from small to big numbers.
static inline int pfib_longest_path(const struct pfib_dag *g) {
    static int len[PFIB_MAX_V];
    int best = 0;
    for (int v = 0; v < g->nv; v++) len[v] = 1;
    // visit vertices in number order: all of v's predecessors have smaller numbers
    for (int v = 0; v < g->nv; v++) {
        for (int e = 0; e < g->ne; e++)
            if (g->edges[e][1] == v && len[g->edges[e][0]] + 1 > len[v]) len[v] = len[g->edges[e][0]] + 1;
        if (len[v] > best) best = len[v];
    }
    return best;
}
#endif
