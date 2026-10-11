// TIMEOUT: 30
#include "harness.h"
#include "twins.h"

long count_twins_serial(long N, long *work);
void block_range(long M, int T, int t, long *first, long *count);
int cyclic_owner(long p, int T);
double imbalance(const long *work, int T);

static void test_serial(void) {
    long w = -1;
    CHECK_EQ(count_twins_serial(100, &w), 8);          // (3,5) (5,7) (11,13) (17,19) (29,31) (41,43) (59,61) (71,73)
    CHECK_EQ(w, 243);
    CHECK_EQ(count_twins_serial(4, &w), 0);            // (3,5) needs N >= 5
    CHECK_EQ(count_twins_serial(5, &w), 1);
    CHECK_EQ(count_twins_serial(1000, &w), 35);
    CHECK_EQ(count_twins_serial(100000, &w), 1224);
    CHECK_EQ(w, 1625620);
}

static void test_block(void) {
    int bad = 0;
    for (long M = 0; M <= 50 && !bad; M++)
        for (int T = 1; T <= 8 && !bad; T++) {
            long next = 0;
            for (int t = 0; t < T; t++) {
                long f = -1, c = -1;
                block_range(M, T, t, &f, &c);
                if (f != next || c < M / T || c > M / T + 1) {
                    CHECK(0, "M=%ld T=%d thread %d: first=%ld count=%ld (expected first=%ld and count %ld or %ld)", M, T, t, f, c, next, M / T, M / T + 1);
                    bad = 1; break;
                }
                next = f + c;
            }
            if (!bad && next != M) { CHECK(0, "M=%ld T=%d: the blocks cover %ld items, expected %ld", M, T, next, M); bad = 1; }
        }
}

static void test_cyclic(void) {
    for (long p = 0; p < 20; p++) CHECK_EQ(cyclic_owner(p, 4), p % 4);
    CHECK_EQ(cyclic_owner(1000001, 7), 1000001 % 7);
}

static void test_imbalance(void) {
    long even[4] = {10, 10, 10, 10}, skew[4] = {10, 10, 10, 50}, zero[3] = {0, 0, 0};
    CHECK_NEAR(imbalance(even, 4), 1.0, 1e-9);
    CHECK_NEAR(imbalance(skew, 4), 50.0 / 20.0, 1e-9);
    CHECK_NEAR(imbalance(zero, 3), 1.0, 1e-9);
}

int main(void) {
    harness_watchdog(20);
    harness_hint("serial: add up tw_item(p, work) for p = 0 .. N-2; imbalance = max / (sum / T)");
    RUN_TEST(test_serial);
    RUN_TEST(test_block);
    RUN_TEST(test_cyclic);
    RUN_TEST(test_imbalance);
    return harness_report();
}
