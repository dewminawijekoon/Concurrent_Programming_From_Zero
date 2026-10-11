// TIMEOUT: 60
#include "harness.h"
#include "ex2_lost_update.h"
#include "race_window.h"

#define THREADS 4
static struct account the_account;
static long per_thread;
static atomic_long successes;

static void depositor(int id, void *ctx) {
    (void)id; (void)ctx;
    for (long k = 0; k < per_thread; k++) deposit(&the_account, 1);
}

static void withdrawer(int id, void *ctx) {
    (void)id; (void)ctx;
    for (long k = 0; k < per_thread; k++)
        if (withdraw(&the_account, 1)) atomic_fetch_add(&successes, 1);
}

static void mixed(int id, void *ctx) {
    if (id % 2 == 0) depositor(id, ctx);
    else withdrawer(id, ctx);
}

// Lost update: 4 threads x N deposits of 1 must give exactly 4N.
static void test_concurrent_deposits(void) {
    account_init(&the_account, 0);
    per_thread = harness_iters(20000);
    long calls_before = atomic_load(&race_window_calls);
    harness_parallel(THREADS, depositor, NULL);
    long want = THREADS * per_thread, got = account_balance(&the_account);
    CHECK(got == want, "balance is %ld after %ld deposits of 1: %ld deposits were lost "
                       "(two threads read the same old balance)", got, want, want - got);
    CHECK(atomic_load(&race_window_calls) - calls_before >= want,
          "deposit() no longer calls race_window(). Keep it: the fix is a lock, not a "
          "smaller window");
}

// Check-then-act: start with 1000, 4 threads try to take 1 many times.
// Exactly 1000 withdrawals may succeed, and the balance must end at 0.
static void test_withdraw_never_overdraws(void) {
    account_init(&the_account, 1000);
    per_thread = 1000;
    atomic_store(&successes, 0);
    harness_parallel(THREADS, withdrawer, NULL);
    long got = account_balance(&the_account), ok = atomic_load(&successes);
    CHECK(got >= 0, "balance went NEGATIVE (%ld): several threads passed the "
                    "\"enough money?\" check before any of them subtracted", got);
    CHECK(ok == 1000, "%ld withdrawals of 1 succeeded from a balance of 1000, expected exactly 1000", ok);
    CHECK(got == 0, "final balance %ld, expected 0", got);
}

// Deposits and withdrawals at the same time: money is never created or lost.
static void test_mixed_conserves_money(void) {
    account_init(&the_account, 500);
    per_thread = harness_iters(10000);
    atomic_store(&successes, 0);
    harness_parallel(THREADS, mixed, NULL);
    long deposits = (THREADS / 2) * per_thread;
    long want = 500 + deposits - atomic_load(&successes);
    long got = account_balance(&the_account);
    CHECK(got == want, "balance %ld, but start + deposits - successful withdrawals = %ld", got, want);
    CHECK(got >= 0, "balance went negative: %ld", got);
}

int main(void) {
    harness_watchdog(50);
    harness_hint("Lock a->lock before reading balance and unlock after writing it. In withdraw(), "
                 "the check and the subtraction must both be inside the lock, and every path "
                 "out of the function must unlock.");
    RUN_TEST(test_concurrent_deposits);
    RUN_TEST(test_withdraw_never_overdraws);
    RUN_TEST(test_mixed_conserves_money);
    if (h_tests_failed == 0)
        printf("  (no lost update or overdraft observed. A test cannot prove the code correct, "
               "but with race_window() yielding inside every critical section, an unprotected "
               "version fails almost every run)\n");
    return harness_report();
}
