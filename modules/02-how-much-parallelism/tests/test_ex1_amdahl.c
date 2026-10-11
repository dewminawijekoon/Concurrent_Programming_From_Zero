// TIMEOUT: 20
#include <math.h>

#include "harness.h"
#include "ex1_amdahl.h"

static void test_speedup_and_efficiency(void) {
    CHECK_NEAR(speedup(100.0, 25.0), 4.0, 1e-12);
    CHECK_NEAR(speedup(12.0, 5.0), 2.4, 1e-12);
    CHECK_NEAR(efficiency(4.0, 8), 0.5, 1e-12);
    CHECK_NEAR(efficiency(2.4, 3), 0.8, 1e-12);
}

// The four examples from L02 slides 6-9 (10 processors).
static void test_lecture_examples(void) {
    CHECK_NEAR(amdahl_speedup(0.60, 10), 2.17, 0.005);
    CHECK_NEAR(amdahl_speedup(0.80, 10), 3.57, 0.005);
    CHECK_NEAR(amdahl_speedup(0.90, 10), 5.26, 0.005);
    CHECK_NEAR(amdahl_speedup(0.99, 10), 9.17, 0.005);
}

static void test_amdahl_edge_cases(void) {
    CHECK_NEAR(amdahl_speedup(0.0, 64), 1.0, 1e-12);    // nothing parallel: no gain
    CHECK_NEAR(amdahl_speedup(1.0, 64), 64.0, 1e-9);    // all parallel: perfect speedup
    CHECK_NEAR(amdahl_speedup(0.75, 1), 1.0, 1e-12);    // one processor: no gain
    CHECK_NEAR(amdahl_speedup(0.5, 3), 1.5, 1e-12);     // needs p / n in double
}

static void test_limit(void) {
    CHECK_NEAR(amdahl_limit(0.95), 20.0, 1e-9);   // L02 slide 10: 5% serial -> at most 20
    CHECK_NEAR(amdahl_limit(0.90), 10.0, 1e-9);
    CHECK_NEAR(amdahl_limit(0.0), 1.0, 1e-12);
    CHECK(isinf(amdahl_limit(1.0)), "amdahl_limit(1.0) should be INFINITY, got %g", amdahl_limit(1.0));
}

static void test_processors_needed(void) {
    CHECK_EQ(processors_needed(0.90, 5.0), 9);    // 8 gives 4.71, 9 gives 5.0
    CHECK_EQ(processors_needed(0.90, 4.0), 6);    // exactly 4.0 at n = 6
    CHECK_EQ(processors_needed(0.95, 10.0), 19);  // exactly 10.0 at n = 19 (watch rounding!)
    CHECK_EQ(processors_needed(0.80, 1.0), 1);
    CHECK_EQ(processors_needed(0.80, 5.0), -1);   // limit is exactly 5: never reached
    CHECK_EQ(processors_needed(0.80, 6.0), -1);
    CHECK_EQ(processors_needed(1.0, 100.0), 100);
}

int main(void) {
    harness_watchdog(10);
    harness_hint("Amdahl: 1.0 / ((1.0 - p) + p / n). For processors_needed, compare with a small "
                 "tolerance: speedup >= target - 1e-9.");
    RUN_TEST(test_speedup_and_efficiency);
    RUN_TEST(test_lecture_examples);
    RUN_TEST(test_amdahl_edge_cases);
    RUN_TEST(test_limit);
    RUN_TEST(test_processors_needed);
    return harness_report();
}
