// Safety-check tests shared by ex3 and ex4 (is_safe only).
#include "deadlock.h"
#include "harness.h"

// Replays a claimed safe sequence and checks it really works.
static bool sequence_works(int n, int m, const int avail[MAXR], const int max[MAXP][MAXR],
                           const int alloc[MAXP][MAXR], const int seq[MAXP]) {
    int work[MAXR];
    bool used[MAXP] = {false};
    for (int r = 0; r < m; r++) work[r] = avail[r];
    for (int k = 0; k < n; k++) {
        int p = seq[k];
        if (p < 0 || p >= n || used[p]) return false;
        for (int r = 0; r < m; r++) if (max[p][r] - alloc[p][r] > work[r]) return false;
        for (int r = 0; r < m; r++) work[r] += alloc[p][r];
        used[p] = true;
    }
    return true;
}

// One resource type, 10 units: A 3/9, B 2/4, C 2/7, so 3 are free.
// Safe: B (needs 2) -> free 5 -> C (needs 5) -> free 7 -> A (needs 6).
static void test_one_resource_safe(void) {
    int avail[MAXR] = {3};
    int max[MAXP][MAXR] = {{9}, {4}, {7}};
    int alloc[MAXP][MAXR] = {{3}, {2}, {2}};
    int seq[MAXP] = {-1, -1, -1};
    CHECK(is_safe(3, 1, avail, max, alloc, seq), "this state is safe (order B, C, A works)");
    CHECK(sequence_works(3, 1, avail, max, alloc, seq),
          "the sequence you returned (%d %d %d) does not work when replayed", seq[0], seq[1], seq[2]);
}

// Same, but A holds 4 (free 2): after B finishes there are 4 free, and
// both A and C still need 5. Unsafe.
static void test_one_resource_unsafe(void) {
    int avail[MAXR] = {2};
    int max[MAXP][MAXR] = {{9}, {4}, {7}};
    int alloc[MAXP][MAXR] = {{4}, {2}, {2}};
    int seq[MAXP];
    CHECK(!is_safe(3, 1, avail, max, alloc, seq),
          "unsafe: after B finishes only 4 are free, but A and C each still need 5");
}

// calc/examples/safe.txt: safe, e.g. P4 P1 P2 P3 P5.
static void test_three_resources_safe(void) {
    int avail[MAXR] = {3, 1, 2};
    int max[MAXP][MAXR] = {{5, 2, 2}, {3, 2, 3}, {5, 3, 3}, {6, 1, 2}, {2, 4, 2}};
    int alloc[MAXP][MAXR] = {{1, 1, 0}, {2, 0, 1}, {0, 2, 2}, {3, 0, 1}, {1, 1, 1}};
    int seq[MAXP] = {-1, -1, -1, -1, -1};
    CHECK(is_safe(5, 3, avail, max, alloc, seq), "this state is safe (P4 P1 P2 P3 P5 works)");
    CHECK(sequence_works(5, 3, avail, max, alloc, seq),
          "the sequence you returned does not work when replayed");
}

// calc/examples/unsafe.txt: one fewer R1 free; nobody's need fits.
static void test_three_resources_unsafe(void) {
    int avail[MAXR] = {2, 1, 2};
    int max[MAXP][MAXR] = {{5, 2, 2}, {3, 2, 3}, {5, 3, 3}, {6, 1, 2}, {2, 4, 2}};
    int alloc[MAXP][MAXR] = {{1, 1, 0}, {2, 0, 1}, {0, 2, 2}, {3, 0, 1}, {1, 1, 1}};
    int seq[MAXP];
    CHECK(!is_safe(5, 3, avail, max, alloc, seq), "unsafe: no process's Need fits in (2 1 2)");
}

// Nobody needs anything more: trivially safe.
static void test_nothing_needed(void) {
    int avail[MAXR] = {0, 0};
    int max[MAXP][MAXR] = {{1, 0}, {0, 2}};
    int alloc[MAXP][MAXR] = {{1, 0}, {0, 2}};
    int seq[MAXP] = {-1, -1};
    CHECK(is_safe(2, 2, avail, max, alloc, seq), "nobody needs more, so the state is safe");
    CHECK(sequence_works(2, 2, avail, max, alloc, seq), "the sequence must list both processes");
}

static void run_safety_tests(void) {
    harness_watchdog(10);
    harness_hint("Need = Max - Alloc. work = Available. Repeatedly pick an unfinished process with "
                 "Need <= work, add its Alloc to work, and append it to the sequence.");
    RUN_TEST(test_one_resource_safe);
    RUN_TEST(test_one_resource_unsafe);
    RUN_TEST(test_three_resources_safe);
    RUN_TEST(test_three_resources_unsafe);
    RUN_TEST(test_nothing_needed);
}
