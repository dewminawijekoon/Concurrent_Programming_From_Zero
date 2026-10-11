// TIMEOUT: 30
#include "harness.h"
#include "ex3_workspan.h"
#include "pfib_dag.h"

static double ones[PFIB_MAX_V];
static struct pfib_dag g;

static void test_small_weighted_dag(void) {
    // A(2) -> B(3), A -> C(1), B -> D(4), C -> D
    double cost[] = {2, 3, 1, 4};
    int edges[][2] = {{0, 1}, {0, 2}, {1, 3}, {2, 3}};
    struct dag_measures m = workspan(4, cost, 4, edges);
    CHECK_NEAR(m.work, 10.0, 1e-9);
    CHECK_NEAR(m.span, 9.0, 1e-9);              // A -> B -> D = 2 + 3 + 4
    CHECK_NEAR(m.parallelism, 10.0 / 9.0, 1e-9);
}

static void test_chain_and_independent(void) {
    double c5[] = {1, 1, 1, 1, 1};
    int chain[][2] = {{0, 1}, {1, 2}, {2, 3}, {3, 4}};
    struct dag_measures m = workspan(5, c5, 4, chain);
    CHECK(m.work == 5 && m.span == 5, "a chain of 5 unit tasks: work %g span %g, expected 5 and 5",
          m.work, m.span);
    m = workspan(5, c5, 0, chain);              // no edges: all 5 can run at once
    CHECK(m.work == 5 && m.span == 1 && m.parallelism == 5,
          "5 independent unit tasks: work %g span %g parallelism %g, expected 5, 1, 5",
          m.work, m.span, m.parallelism);
}

static void test_edges_listed_in_any_order(void) {
    // Same DAG as the first test, edges shuffled: the answer must not change.
    double cost[] = {2, 3, 1, 4};
    int edges[][2] = {{2, 3}, {1, 3}, {0, 2}, {0, 1}};
    struct dag_measures m = workspan(4, cost, 4, edges);
    CHECK_NEAR(m.span, 9.0, 1e-9);
}

static void test_bad_numbering(void) {
    double cost[] = {1, 1, 1};
    int back[][2] = {{0, 1}, {2, 1}};           // 2 -> 1 breaks the u < v rule
    struct dag_measures m = workspan(3, cost, 2, back);
    CHECK(m.work == -1 && m.span == -1 && m.parallelism == -1,
          "an edge 2 -> 1 should give {-1, -1, -1}, got {%g, %g, %g}", m.work, m.span, m.parallelism);
    int out[][2] = {{0, 7}};                    // vertex 7 does not exist
    m = workspan(3, cost, 1, out);
    CHECK(m.work == -1, "an edge to a vertex that does not exist should give -1, got work %g", m.work);
}

// L02 slides 21 and 26: P-FIB(4) has work 17, span 8, parallelism 2.125.
static void test_pfib4_lecture(void) {
    pfib_make(4, &g);
    struct dag_measures m = workspan(g.nv, ones, g.ne, (const int(*)[2])g.edges);
    CHECK_NEAR(m.work, 17.0, 1e-9);
    CHECK_NEAR(m.span, 8.0, 1e-9);
    CHECK_NEAR(m.parallelism, 2.125, 1e-9);
}

static void test_pfib10(void) {
    pfib_make(10, &g);
    struct dag_measures m = workspan(g.nv, ones, g.ne, (const int(*)[2])g.edges);
    CHECK_NEAR(m.work, 353.0, 1e-9);
    CHECK_NEAR(m.span, 20.0, 1e-9);
}

static void test_lower_bound(void) {
    CHECK_NEAR(tp_lower_bound(17, 8, 1), 17.0, 1e-12);   // work law wins
    CHECK_NEAR(tp_lower_bound(17, 8, 2), 8.5, 1e-12);
    CHECK_NEAR(tp_lower_bound(17, 8, 4), 8.0, 1e-12);    // span law wins
    CHECK_NEAR(tp_lower_bound(100, 3, 10), 10.0, 1e-12);
}

int main(void) {
    harness_watchdog(20);
    for (int i = 0; i < PFIB_MAX_V; i++) ones[i] = 1.0;
    harness_hint("finish[v] = cost[v] + max over edges u->v of finish[u]; go through v = 0, 1, 2, ... "
                 "in order and scan ALL edges for each v (they may be listed in any order).");
    RUN_TEST(test_small_weighted_dag);
    RUN_TEST(test_chain_and_independent);
    RUN_TEST(test_edges_listed_in_any_order);
    RUN_TEST(test_bad_numbering);
    RUN_TEST(test_pfib4_lecture);
    RUN_TEST(test_pfib10);
    RUN_TEST(test_lower_bound);
    return harness_report();
}
