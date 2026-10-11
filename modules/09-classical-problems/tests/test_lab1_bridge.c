// TIMEOUT: 60
// Tests for lab track 1 (Lab 04 Ex 1): one direction at a time, at most N cars.
void bridge_init(int max_cars);
void bridge_destroy(void);
void bridge_enter(int dir);
void bridge_exit(int dir);
#define BR_INIT(c) bridge_init(c)
#define BR_DESTROY() bridge_destroy()
#define BR_ENTER(d) bridge_enter(d)
#define BR_EXIT(d) bridge_exit(d)
#define BR_NAME_0 "N->S"
#define BR_NAME_1 "S->N"
#include "bridge_tests.h"

int main(void) {
    harness_watchdog(50);
    harness_hint("One lightswitch per direction on a 'bridge free' semaphore (rule a), plus a "
                 "semaphore that starts at the capacity (rule b). Lightswitch first, then capacity.");
    RUN_TEST(test_opposite_direction_waits);
    RUN_TEST(test_same_direction_shares_up_to_capacity);
    RUN_TEST(test_capacity_one);
    RUN_TEST(test_stress_capacity_3);
    RUN_TEST(test_stress_capacity_1);
    RUN_TEST(test_stress_capacity_5);
    return harness_report();
}
