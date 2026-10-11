// TIMEOUT: 20
#include "deadlock.h"
#include "harness.h"

static bool dead[MAXP];

static int names_to_mask(void) {      // bit p set if P(p+1) is reported deadlocked
    int mask = 0;
    for (int p = 0; p < MAXP; p++) if (dead[p]) mask |= 1 << p;
    return mask;
}

// The example from the exercise header. A starts at (0, 0), yet nobody is
// deadlocked: P3 asks for nothing, finishes, and frees enough for the rest.
static void test_nothing_free_but_no_deadlock(void) {
    int E[MAXR] = {4, 2};
    int C[MAXP][MAXR] = {{2, 0}, {1, 1}, {1, 1}};
    int R[MAXP][MAXR] = {{0, 1}, {1, 0}, {0, 0}};
    CHECK_EQ(detect_deadlock(3, 2, E, C, R, dead), 0);
    CHECK(names_to_mask() == 0, "no process should be marked deadlocked");
}

// calc/examples/detect.txt: P3 can finish, but P1, P2 and P4 are stuck.
static void test_three_of_four_deadlocked(void) {
    int E[MAXR] = {4, 4, 3};
    int C[MAXP][MAXR] = {{2, 1, 0}, {1, 2, 1}, {0, 1, 1}, {1, 0, 0}};
    int R[MAXP][MAXR] = {{0, 2, 0}, {1, 0, 0}, {0, 0, 1}, {1, 0, 1}};
    int C0[MAXP][MAXR], R0[MAXP][MAXR], E0[MAXR];
    memcpy(C0, C, sizeof C); memcpy(R0, R, sizeof R); memcpy(E0, E, sizeof E);
    int k = detect_deadlock(4, 3, E, C, R, dead);
    CHECK(k == 3, "expected 3 deadlocked processes (P1, P2, P4), got %d", k);
    CHECK(names_to_mask() == 0xB, "expected exactly P1, P2 and P4 deadlocked (P3 can finish)");
    CHECK(memcmp(C, C0, sizeof C) == 0 && memcmp(R, R0, sizeof R) == 0 && memcmp(E, E0, sizeof E) == 0,
          "detect_deadlock changed the caller's tables; work on copies");
}

// calc/examples/detect_ok.txt: P3, then P2, then P1, then P4 can finish.
// P1 and P2 come before P3 in the list, so ONE pass over the list is not
// enough: after a process finishes, look at the others again.
static void test_needs_several_passes(void) {
    int E[MAXR] = {5, 4, 3};
    int C[MAXP][MAXR] = {{2, 1, 0}, {1, 2, 1}, {1, 1, 1}, {1, 0, 0}};
    int R[MAXP][MAXR] = {{0, 2, 0}, {1, 0, 0}, {0, 0, 1}, {1, 0, 1}};
    int k = detect_deadlock(4, 3, E, C, R, dead);
    CHECK(k == 0, "expected no deadlock (order P3, P2, P1, P4 works), got %d deadlocked", k);
}

// Each holds all of one resource and wants the other's.
static void test_classic_two_process_deadlock(void) {
    int E[MAXR] = {1, 1};
    int C[MAXP][MAXR] = {{1, 0}, {0, 1}};
    int R[MAXP][MAXR] = {{0, 1}, {1, 0}};
    CHECK_EQ(detect_deadlock(2, 2, E, C, R, dead), 2);
    CHECK(dead[0] && dead[1], "both processes should be marked deadlocked");
}

int main(void) {
    harness_watchdog(10);
    harness_hint("A = E minus the column sums of C. Repeat: find an unmarked process whose whole "
                 "request row fits in A, add its C row to A, mark it. Unmarked at the end = deadlocked.");
    RUN_TEST(test_nothing_free_but_no_deadlock);
    RUN_TEST(test_three_of_four_deadlocked);
    RUN_TEST(test_needs_several_passes);
    RUN_TEST(test_classic_two_process_deadlock);
    return harness_report();
}
