// ANSWER - Exercise 4 (from memory) - the Banker's safety check.
#include "deadlock.h"

bool is_safe(int n, int m, const int available[MAXR], const int max[MAXP][MAXR],
             const int alloc[MAXP][MAXR], int sequence[MAXP]) {
    int work[MAXR], need[MAXP][MAXR];
    bool finished[MAXP] = {false};
    for (int r = 0; r < m; r++) work[r] = available[r];
    for (int p = 0; p < n; p++)
        for (int r = 0; r < m; r++) need[p][r] = max[p][r] - alloc[p][r];
    for (int k = 0; k < n; k++) {          // pick one finisher per round
        int p = 0;
        for (; p < n; p++) {
            if (finished[p]) continue;
            int r = 0;
            while (r < m && need[p][r] <= work[r]) r++;
            if (r == m) break;             // all of p's needs fit
        }
        if (p == n) return false;          // nobody fits: unsafe
        for (int r = 0; r < m; r++) work[r] += alloc[p][r];
        finished[p] = true;
        sequence[k] = p;
    }
    return true;
}
