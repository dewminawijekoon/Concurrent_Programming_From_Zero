// TIMEOUT: 60
#include "harness.h"
#include "lab2_mutex_vs_local.h"

static void check_a(int n, long m) {
    reset_lock_acquisitions();
    long got = count_global_mutex(n, m);
    long locks = lock_acquisitions();
    CHECK(got == n * m, "Part A, N=%d, M=%ld: counter = %ld, expected %ld", n, m, got, n * m);
    CHECK(locks == n * m, "Part A, N=%d, M=%ld: the lock was taken %ld times, expected %ld "
                          "(once per increment, with counted_lock)", n, m, locks, n * m);
}

static void check_b(int n, long m) {
    reset_lock_acquisitions();
    long got = count_local_then_add(n, m);
    long locks = lock_acquisitions();
    CHECK(got == n * m, "Part B, N=%d, M=%ld: counter = %ld, expected %ld", n, m, got, n * m);
    CHECK(locks == n, "Part B, N=%d, M=%ld: the lock was taken %ld times, expected %d "
                      "(once per thread, at the end, with counted_lock)", n, m, locks, n);
}

static void test_global_mutex(void) {
    check_a(1, 50000);
    check_a(4, 50000);
    check_a(8, 20000);
    check_a(4, 50000);   // again: counter must be reset to 0 at the start
}

static void test_local_then_add(void) {
    check_b(1, 100000);
    check_b(4, 100000);
    check_b(16, 100000);
    check_b(4, 100000);
}

int main(void) {
    harness_watchdog(50);
    harness_hint("Use counted_lock/counted_unlock around the shared counter. A timeout means a "
                 "lock was never unlocked. Reset counter to 0 at the start of each function.");
    RUN_TEST(test_global_mutex);
    RUN_TEST(test_local_then_add);
    if (h_tests_failed == 0)
        printf("  (both versions gave exact results in every run tried. Timing is not tested; "
               "run `make run M=01 EX=lab2_mutex_vs_local` for the Q3.1 table)\n");
    return harness_report();
}
