// BUILD:
// REQUIRES:
// TIMEOUT: 30
#include "harness.h"

int example_answer(void);

static void test_answer(void) { CHECK_EQ(example_answer(), 42); }

int main(void) {
    harness_watchdog(20);
    harness_hint("what to check if this fails");
    RUN_TEST(test_answer);
    return harness_report();
}
