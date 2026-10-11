// Interface for ex3_workspan.c (shared by the exercise and its test)
#ifndef EX3_WORKSPAN_H
#define EX3_WORKSPAN_H

#define DAG_MAX_VERTICES 4096

struct dag_measures {
    double work;         // T1: sum of all costs
    double span;         // T-infinity: cost of the most expensive path
    double parallelism;  // work / span
};

// Vertices 0..nv-1 with cost[v]; edges[e] = {u, v} means u before v, with u < v.
// Returns {-1, -1, -1} if an edge breaks the u < v rule or is out of range.
struct dag_measures workspan(int nv, const double cost[], int ne, const int edges[][2]);

// max(work / p, span): no schedule on p processors can be faster.
double tp_lower_bound(double work, double span, int p);
#endif
