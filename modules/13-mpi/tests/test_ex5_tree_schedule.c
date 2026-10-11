// TIMEOUT: 20
#include "harness.h"

enum { TREE_IDLE = 0, TREE_SEND = 1, TREE_RECV = 2 };
int tree_steps(int p);
int tree_role(int rank, int step, int p, int *partner);
int butterfly_partner(int rank, int step);

static void test_steps(void) {
    CHECK_EQ(tree_steps(1), 0);
    CHECK_EQ(tree_steps(2), 1);
    CHECK_EQ(tree_steps(4), 2);
    CHECK_EQ(tree_steps(5), 3);
    CHECK_EQ(tree_steps(8), 3);
    CHECK_EQ(tree_steps(1000), 10);
}

static void test_slide_example(void) {
    int partner;
    CHECK(tree_role(1, 0, 8, &partner) == TREE_SEND && partner == 0, "p=8 round 0: rank 1 must send to rank 0");
    CHECK(tree_role(0, 0, 8, &partner) == TREE_RECV && partner == 1, "p=8 round 0: rank 0 must receive from rank 1");
    CHECK(tree_role(6, 1, 8, &partner) == TREE_SEND && partner == 4, "p=8 round 1: rank 6 must send to rank 4");
    CHECK(tree_role(4, 2, 8, &partner) == TREE_SEND && partner == 0, "p=8 round 2: rank 4 must send to rank 0");
    CHECK(tree_role(3, 2, 8, &partner) == TREE_IDLE, "p=8 round 2: rank 3 sent its sum in round 0, so it is idle");
    CHECK(tree_role(4, 0, 5, &partner) == TREE_IDLE, "p=5 round 0: rank 4 has no partner 5, so it is idle");
}

// Simulate the tree sum with the student's rules for p = 1..40.
static void test_simulated_sum(void) {
    for (int p = 1; p <= 40; p++) {
        long val[64];
        int done[64] = {0}, sends[64] = {0};
        for (int r = 0; r < p; r++) val[r] = r + 1;
        int steps = tree_steps(p), ok = 1;
        for (int s = 0; s < steps && ok; s++) {
            for (int r = 0; r < p && ok; r++) {
                int partner = -2, role = tree_role(r, s, p, &partner);
                if (role == TREE_SEND) {
                    int back = -2;
                    if (partner < 0 || partner >= p || tree_role(partner, s, p, &back) != TREE_RECV || back != r) {
                        CHECK(0, "p=%d round %d: rank %d sends to %d, but %d is not receiving from %d in that round", p, s, r, partner, partner, r);
                        ok = 0; break;
                    }
                    if (done[r]) { CHECK(0, "p=%d round %d: rank %d sends again after it already sent its sum", p, s, r); ok = 0; break; }
                    val[partner] += val[r];
                    done[r] = 1;
                    sends[r]++;
                } else if (role == TREE_RECV) {
                    int back = -2;
                    if (partner < 0 || partner >= p || tree_role(partner, s, p, &back) != TREE_SEND || back != r) {
                        CHECK(0, "p=%d round %d: rank %d waits for %d, but %d is not sending to %d (that Recv would hang)", p, s, r, partner, partner, r);
                        ok = 0; break;
                    }
                    if (done[r]) { CHECK(0, "p=%d round %d: rank %d receives after it already sent its sum away", p, s, r); ok = 0; break; }
                }
            }
        }
        if (!ok) return;
        long want = (long)p * (p + 1) / 2;
        CHECK(val[0] == want, "p=%d: rank 0 ends with %ld, expected the total %ld", p, val[0], want);
        int unsent = 0;
        for (int r = 1; r < p; r++) unsent += sends[r] != 1;
        CHECK(unsent == 0, "p=%d: %d ranks other than 0 did not send exactly once", p, unsent);
        if (val[0] != want || unsent) return;
    }
}

static void test_butterfly(void) {
    CHECK_EQ(butterfly_partner(0, 0), 1);
    CHECK_EQ(butterfly_partner(5, 1), 7);
    CHECK_EQ(butterfly_partner(6, 2), 2);
    for (int p = 1; p <= 32; p *= 2) {
        long val[32], next[32];
        for (int r = 0; r < p; r++) val[r] = 10 * r + 1;
        for (int s = 0; (1 << s) < p; s++) {
            for (int r = 0; r < p; r++) {
                int q = butterfly_partner(r, s);
                if (q < 0 || q >= p) { CHECK(0, "p=%d round %d: butterfly partner of %d is %d (out of range)", p, s, r, q); return; }
                next[r] = val[r] + val[q];
            }
            for (int r = 0; r < p; r++) val[r] = next[r];
        }
        long want = 0;
        for (int r = 0; r < p; r++) want += 10 * r + 1;
        int wrong = 0;
        for (int r = 0; r < p; r++) wrong += val[r] != want;
        CHECK(wrong == 0, "butterfly with p=%d: %d ranks do not end with the total %ld", p, wrong, want);
    }
}

int main(void) {
    harness_watchdog(10);
    harness_hint("round s: d = 1 << s; rank %% (2d) == 0 receives from rank + d (if it exists); rank %% (2d) == d sends to rank - d");
    RUN_TEST(test_steps);
    RUN_TEST(test_slide_example);
    RUN_TEST(test_simulated_sum);
    RUN_TEST(test_butterfly);
    return harness_report();
}
