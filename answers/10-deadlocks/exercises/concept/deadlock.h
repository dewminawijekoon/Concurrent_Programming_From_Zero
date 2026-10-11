// deadlock.h - shared sizes and interfaces for the module 10 algorithm exercises.
#ifndef DEADLOCK_H
#define DEADLOCK_H
#include <stdbool.h>

#define MAXP 8    // at most 8 processes (rows)
#define MAXR 8    // at most 8 resource types (columns)
#define MAXN 16   // at most 16 nodes in a resource-allocation graph

// ex1: resource-allocation graph with ONE instance of each resource.
// Nodes 0..n-1 are processes and resources mixed; edge[u][v] == true means
// an arrow u -> v ("process u waits for resource v", or "resource u is held
// by process v"). Returns the length k of some cycle and writes its nodes
// into cycle[0..k-1] (cycle[0] -> cycle[1] -> ... -> cycle[k-1] -> cycle[0]).
// Returns 0 if the graph has no cycle.
int find_cycle(int n, const bool edge[MAXN][MAXN], int cycle[MAXN]);

// ex2: deadlock detection with several instances of each resource type.
//   existing[r]   E: how many instances of resource r exist in total
//   alloc[p][r]   C: how many instances of r process p holds right now
//   request[p][r] R: how many more instances of r process p is waiting for
// Sets deadlocked[p] = true for every process that can never finish, and
// returns how many there are (0 = no deadlock).
int detect_deadlock(int n, int m, const int existing[MAXR], const int alloc[MAXP][MAXR],
                    const int request[MAXP][MAXR], bool deadlocked[MAXP]);

// ex3, ex4: Banker's algorithm.
//   available[r]  how many instances of r are free right now
//   max[p][r]     the most instances of r process p may ever hold at once
//   alloc[p][r]   how many it holds now
// is_safe: returns true if every process can finish in some order, and
// writes one such order (process numbers) into sequence[0..n-1].
bool is_safe(int n, int m, const int available[MAXR], const int max[MAXP][MAXR],
             const int alloc[MAXP][MAXR], int sequence[MAXP]);

// ex3 only: process p asks for req[]. Grant it (update available and
// alloc, return true) only if the request is within p's declared maximum,
// enough is free, and the state after granting is safe. Otherwise change
// nothing and return false.
bool request_ok(int n, int m, int available[MAXR], const int max[MAXP][MAXR],
                int alloc[MAXP][MAXR], int p, const int req[MAXR]);
#endif
