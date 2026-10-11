// TIMEOUT: 60
#include "atomics.h"
#include "harness.h"

extern long counter_plain;
extern shared_long counter_atomic;
extern shared_long cas_failures;
void inc_mutex(long n);
long cas_get_and_increment(shared_long *c);
void inc_cas(long n);
void inc_faa(long n);
// inc_unsafe is not tested: it is wrong on purpose (and ThreadSanitizer would complain).

#define T 4
static long iters;
static void (*fn)(long);
static void run(int id, void *ctx) {
    (void)id; (void)ctx;
    fn(iters);
}

static void test_mutex(void) {
    iters = harness_iters(200000);
    counter_plain = 0;
    fn = inc_mutex;
    harness_parallel(T, run, NULL);
    CHECK(counter_plain == T * iters, "mutex version: counter = %ld, expected %ld", counter_plain,
          T * iters);
}

static void test_cas_single_thread(void) {
    STORE(counter_atomic, 40);
    STORE(cas_failures, 0);
    CHECK_EQ(cas_get_and_increment(&counter_atomic), 40);   // returns the OLD value
    CHECK_EQ(cas_get_and_increment(&counter_atomic), 41);
    CHECK_EQ(LOAD(counter_atomic), 42);
    CHECK(LOAD(cas_failures) == 0, "one thread alone: no CAS can fail, but %ld failures were "
                                   "counted", LOAD(cas_failures));
}

static void test_cas(void) {
    iters = harness_iters(200000);
    STORE(counter_atomic, 0);
    STORE(cas_failures, 0);
    fn = inc_cas;
    harness_parallel(T, run, NULL);
    CHECK(LOAD(counter_atomic) == T * iters, "CAS version: counter = %ld, expected %ld",
          LOAD(counter_atomic), T * iters);
    printf("    (%ld of the CAS attempts failed and were retried)\n", LOAD(cas_failures));
}

static void test_faa(void) {
    iters = harness_iters(200000);
    STORE(counter_atomic, 0);
    fn = inc_faa;
    harness_parallel(T, run, NULL);
    CHECK(LOAD(counter_atomic) == T * iters, "fetch-and-add version: counter = %ld, expected %ld",
          LOAD(counter_atomic), T * iters);
}

// getAndIncrement hands out tickets (L01's prime counter): every number
// 0 .. T*N-1 must be handed out exactly once.
#define TICKETS 50000
static atomic_int seen[T * TICKETS];
static atomic_int bad_ticket;
static void ticket_worker(int id, void *ctx) {
    (void)id; (void)ctx;
    for (int i = 0; i < TICKETS; i++) {
        long v = cas_get_and_increment(&counter_atomic);
        if (v < 0 || v >= T * TICKETS) atomic_store(&bad_ticket, 1);
        else atomic_fetch_add(&seen[v], 1);
    }
}

static void test_tickets_unique(void) {
    STORE(counter_atomic, 0);
    for (int i = 0; i < T * TICKETS; i++) atomic_store(&seen[i], 0);
    harness_parallel(T, ticket_worker, NULL);
    CHECK(!atomic_load(&bad_ticket), "cas_get_and_increment returned a value outside 0..%d",
          T * TICKETS - 1);
    for (int i = 0; i < T * TICKETS; i++)
        if (atomic_load(&seen[i]) != 1) {
            CHECK(0, "value %d was returned %d times (every value must be returned exactly once)",
                  i, atomic_load(&seen[i]));
            break;
        }
}

int main(void) {
    harness_watchdog(50);
    harness_hint("cas_get_and_increment must return the value it read just before its CAS "
                 "succeeded. Count a failure only when CAS returns false.");
    RUN_TEST(test_mutex);
    RUN_TEST(test_cas_single_thread);
    RUN_TEST(test_cas);
    RUN_TEST(test_faa);
    RUN_TEST(test_tickets_unique);
    return harness_report();
}
