// TIMEOUT: 20
#include "harness.h"
#include "ex2_measuring.h"

static const double t5[] = {18, 22, 20, 25, 15};
static const double t6[] = {10.2, 9.8, 10.0, 10.4, 9.6, 10.0};

static void test_mean(void) {
    CHECK_NEAR(mean(t5, 5), 20.0, 1e-12);
    CHECK_NEAR(mean(t6, 6), 10.0, 1e-12);
    double one[] = {7.5};
    CHECK_NEAR(mean(one, 1), 7.5, 1e-12);
}

static void test_stddev(void) {
    CHECK_NEAR(sample_stddev(t5, 5), 3.807887, 1e-5);   // sqrt(58 / 4): divide by n - 1
    CHECK_NEAR(sample_stddev(t6, 6), 0.282843, 1e-5);
    double same[] = {4, 4, 4};
    CHECK_NEAR(sample_stddev(same, 3), 0.0, 1e-12);
    double one[] = {7.5};
    CHECK_NEAR(sample_stddev(one, 1), 0.0, 1e-12);
}

static void test_confidence_interval(void) {
    double lo = 0, hi = 0;
    confidence_interval(t5, 5, 1.96, &lo, &hi);
    CHECK_NEAR(lo, 16.662240, 1e-4);
    CHECK_NEAR(hi, 23.337760, 1e-4);
    confidence_interval(t6, 6, 2.576, &lo, &hi);        // 99% confidence
    CHECK_NEAR(lo, 10.0 - 0.297451, 1e-4);
    CHECK_NEAR(hi, 10.0 + 0.297451, 1e-4);
}

static void test_samples_needed(void) {
    // L02 slide 36: mean 20 s, s = 5, +/- 1 s = 5%, 95% (z = 1.96): 96.04 -> 97 runs
    CHECK_EQ(samples_needed(1.96, 5, 5, 20), 97);
    CHECK_EQ(samples_needed(2.576, 5, 5, 20), 166);   // 99%: 165.89 -> 166
    CHECK_EQ(samples_needed(1.645, 2, 10, 8), 17);    // 90%: 16.91 -> 17
    CHECK_EQ(samples_needed(2.0, 5, 10, 10), 100);    // exactly 100: must not become 101
}

int main(void) {
    harness_watchdog(10);
    harness_hint("Standard deviation divides by n - 1. Round the sample size UP with ceil(), "
                 "after subtracting a tiny tolerance such as 1e-9.");
    RUN_TEST(test_mean);
    RUN_TEST(test_stddev);
    RUN_TEST(test_confidence_interval);
    RUN_TEST(test_samples_needed);
    return harness_report();
}
