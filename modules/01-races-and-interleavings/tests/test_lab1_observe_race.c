// TIMEOUT: 60
#include "harness.h"
#include "lab1_observe_race.h"

static void test_error_percent(void) {
    CHECK_NEAR(error_percent(300000, 400000), 25.0, 1e-9);
    CHECK_NEAR(error_percent(400000, 400000), 0.0, 1e-9);
    CHECK_NEAR(error_percent(1, 3), 66.666666, 1e-4);   // needs double division, not long
    CHECK_NEAR(error_percent(5, 0), 0.0, 1e-9);
}

// Synthetic "measurements", so the answer is known exactly.
static void test_summarize_runs(void) {
    long r[5] = {380000, 400000, 350000, 399990, 371010};
    struct race_stats s = summarize_runs(r, 5, 400000);
    CHECK_EQ(s.min, 350000);
    CHECK_EQ(s.max, 400000);
    CHECK_EQ(s.range, 50000);
    CHECK_NEAR(s.mean, 380200.0, 1e-6);
    CHECK_NEAR(s.mean_error_pct, 4.95, 1e-9);   // (5 + 0 + 12.5 + 0.0025 + 7.2475) / 5
    CHECK_EQ(s.runs_with_loss, 4);
}

static void test_summarize_single_run(void) {
    long r[1] = {123};
    struct race_stats s = summarize_runs(r, 1, 200);
    CHECK(s.min == 123 && s.max == 123 && s.range == 0,
          "one run of 123: min/max/range should be 123/123/0, got %ld/%ld/%ld", s.min, s.max, s.range);
    CHECK_EQ(s.runs_with_loss, 1);
}

// One thread cannot race with itself, so it must be exact; and every run
// must start again from 0.
static void test_race_once_one_thread(void) {
    CHECK_EQ(race_once(1, 100000), 100000);
    CHECK_EQ(race_once(1, 100000), 100000);
}

// With several threads the result is never MORE than n*m (an update can be
// lost, but never invented). We do not check how much is lost: that depends
// on the machine.
static void test_race_once_many_threads(void) {
    long r = race_once(4, 100000);
    CHECK(r > 0 && r <= 400000, "race_once(4, 100000) = %ld; it must be between 1 and 400000", r);
    printf("    (4 threads x 100000: got %ld, lost %ld updates this time)\n", r, 400000 - r);
}

int main(void) {
    harness_watchdog(50);
    harness_hint("race_once: reset counter to 0 before starting the threads. error_percent: "
                 "divide in double. summarize_runs: start min and max at results[0].");
    RUN_TEST(test_error_percent);
    RUN_TEST(test_summarize_runs);
    RUN_TEST(test_summarize_single_run);
    RUN_TEST(test_race_once_one_thread);
    RUN_TEST(test_race_once_many_threads);
    return harness_report();
}
