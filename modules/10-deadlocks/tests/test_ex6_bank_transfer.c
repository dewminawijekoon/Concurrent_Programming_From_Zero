// TIMEOUT: 30
#include "ex6_bank_transfer.h"
#include "harness.h"

#define NACC 4
#define START 1000
static account acc[NACC];
static long rounds;
static atomic_int stop_audit;
static atomic_long bad_audits, audits;

static void reset(void) {
    for (int i = 0; i < NACC; i++) account_init(&acc[i], i, START);
}

static void test_simple_transfers(void) {
    reset();
    CHECK(transfer(&acc[0], &acc[1], 300), "transfer of 300 from an account with 1000 should succeed");
    CHECK(acc[0].balance == 700 && acc[1].balance == 1300,
          "after moving 300 from account 0 to 1: balances %ld and %ld, expected 700 and 1300",
          acc[0].balance, acc[1].balance);
    CHECK(!transfer(&acc[0], &acc[1], 5000), "a transfer larger than the balance must fail");
    CHECK(acc[0].balance == 700 && acc[1].balance == 1300, "a failed transfer must move nothing");
    CHECK(transfer(&acc[3], &acc[2], 1000), "moving the whole balance (1000) should succeed");
}

// Threads move money in BOTH directions between the same accounts, so the
// naive "lock from, then to" deadlocks. An auditor checks the total while
// they run: money must never appear or disappear.
static void mover(int id, void *ctx) {
    (void)ctx;
    for (long k = 0; k < rounds; k++) {
        int a = (int)(harness_rand() % NACC), b = (int)(harness_rand() % NACC);
        if (a == b) b = (a + 1 + id) % NACC;
        transfer(&acc[a], &acc[b], 1 + (long)(harness_rand() % 50));
    }
}
static void *auditor(void *arg) {
    (void)arg;
    while (!atomic_load(&stop_audit)) {
        if (total_balance(acc, NACC) != NACC * START) atomic_fetch_add(&bad_audits, 1);
        atomic_fetch_add(&audits, 1);
    }
    return NULL;
}

static void test_opposite_directions_under_stress(void) {
    reset();
    rounds = harness_iters(100000);
    atomic_store(&stop_audit, 0);
    pthread_t aud;
    pthread_create(&aud, NULL, auditor, NULL);
    harness_parallel(4, mover, NULL);
    atomic_store(&stop_audit, 1);
    pthread_join(aud, NULL);
    CHECK(total_balance(acc, NACC) == NACC * START, "total money is %ld, expected %d",
          total_balance(acc, NACC), NACC * START);
    CHECK(atomic_load(&bad_audits) == 0, "%ld of %ld audits saw a wrong total while transfers ran",
          atomic_load(&bad_audits), atomic_load(&audits));
    for (int i = 0; i < NACC; i++)
        CHECK(acc[i].balance >= 0, "account %d went negative (%ld)", i, acc[i].balance);
    printf("  (4 threads x %ld transfers in both directions finished; %ld audits all correct)\n",
           rounds, atomic_load(&audits));
}

static void test_transfer_to_self(void) {
    reset();
    CHECK(transfer(&acc[2], &acc[2], 10), "a transfer to the same account should succeed");
    CHECK(acc[2].balance == START, "a transfer to yourself must not change the balance");
}

int main(void) {
    harness_watchdog(12);
    harness_hint("A timeout means a deadlock: lock the account with the smaller id first. For "
                 "from == to, take the lock only once.");
    RUN_TEST(test_simple_transfers);
    RUN_TEST(test_opposite_directions_under_stress);
    RUN_TEST(test_transfer_to_self);
    return harness_report();
}
