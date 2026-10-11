// TIMEOUT: 20
#include "deadlock.h"
#include "harness.h"

static bool e[MAXN][MAXN];
static int cyc[MAXN];

static void clear(void) { memset(e, 0, sizeof e); for (int i = 0; i < MAXN; i++) cyc[i] = -1; }
static void arrow(int u, int v) { e[u][v] = true; }

// Is cyc[0..k-1] really a cycle in e? (distinct nodes, every arrow present)
static bool valid_cycle(int n, int k) {
    if (k < 2 || k > n) return false;
    for (int i = 0; i < k; i++) {
        if (cyc[i] < 0 || cyc[i] >= n) return false;
        for (int j = 0; j < i; j++) if (cyc[j] == cyc[i]) return false;
        if (!e[cyc[i]][cyc[(i + 1) % k]]) return false;
    }
    return true;
}

static bool on_cycle(int k, int node) {
    for (int i = 0; i < k; i++) if (cyc[i] == node) return true;
    return false;
}

// P holds R1 and wants R2; Q holds R2 and wants R1.
static void test_two_processes_two_resources(void) {
    enum { P, Q, R1, R2 };
    clear();
    arrow(R1, P); arrow(P, R2); arrow(R2, Q); arrow(Q, R1);
    int k = find_cycle(4, e, cyc);
    CHECK(k == 4, "expected the cycle P -> R2 -> Q -> R1 (4 nodes), got length %d", k);
    CHECK(valid_cycle(4, k), "the nodes you returned do not form a cycle along the arrows");
}

// A holds R and wants S; B holds S and wants T; C holds T and wants R (a cycle).
// D holds U and wants S (waits for B). E holds V and wants U (waits for D).
// F holds W and wants nothing.
static void test_cycle_with_waiters_hanging_off(void) {
    enum { A, B, C, D, E_, F, R, S, T, U, V, W, N };
    clear();
    arrow(R, A); arrow(A, S);
    arrow(S, B); arrow(B, T);
    arrow(T, C); arrow(C, R);
    arrow(U, D); arrow(D, S);
    arrow(V, E_); arrow(E_, U);
    arrow(W, F);
    int k = find_cycle(N, e, cyc);
    CHECK(k == 6, "expected the 6-node cycle through A, B and C, got length %d", k);
    CHECK(valid_cycle(N, k), "the nodes you returned do not form a cycle along the arrows");
    if (valid_cycle(N, k))
        CHECK(on_cycle(k, A) && on_cycle(k, B) && on_cycle(k, C) && !on_cycle(k, D) &&
              !on_cycle(k, E_) && !on_cycle(k, F),
              "the cycle should contain A, B, C (and R, S, T) and not D, E or F");
}

// A chain of waiting, but nobody waits for the first process: no deadlock.
static void test_no_cycle(void) {
    enum { P, Q, S, R1, R2, R3 };
    clear();
    arrow(R1, P);              // P holds R1 (and is running)
    arrow(Q, R1); arrow(R2, Q);  // Q holds R2, wants R1
    arrow(S, R2); arrow(R3, S);  // S holds R3, wants R2
    CHECK_EQ(find_cycle(6, e, cyc), 0);
}

// Two paths meet at the same node, but there is no cycle. A search that
// only remembers "visited" (not "on my current path") wrongly reports one.
static void test_diamond_is_not_a_cycle(void) {
    clear();
    arrow(0, 1); arrow(0, 2); arrow(1, 3); arrow(2, 3);
    CHECK(find_cycle(4, e, cyc) == 0,
          "0->1->3 and 0->2->3 meet at node 3, but that is not a cycle (no arrow leads back)");
}

// The cycle cannot be reached from node 0: you must start a search from
// every unvisited node.
static void test_cycle_not_reachable_from_node_0(void) {
    clear();
    arrow(0, 1);
    arrow(2, 3); arrow(3, 4); arrow(4, 2);
    int k = find_cycle(5, e, cyc);
    CHECK(k == 3, "expected the cycle 2 -> 3 -> 4, got length %d", k);
    CHECK(valid_cycle(5, k), "the nodes you returned do not form a cycle along the arrows");
}

int main(void) {
    harness_watchdog(10);
    harness_hint("Colour nodes WHITE/GREY/BLACK. A cycle is an arrow to a GREY node (one on the "
                 "current path), not just to any visited node. Start a DFS from every WHITE node.");
    RUN_TEST(test_two_processes_two_resources);
    RUN_TEST(test_cycle_with_waiters_hanging_off);
    RUN_TEST(test_no_cycle);
    RUN_TEST(test_diamond_is_not_a_cycle);
    RUN_TEST(test_cycle_not_reachable_from_node_0);
    return harness_report();
}
