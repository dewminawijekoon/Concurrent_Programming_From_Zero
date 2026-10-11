// TIMEOUT: 60
// Tests for exercise 1: Bill and Ben must each wait for the other's food.
#include "sem_test_util.h"

void cafe_init(void);
void cafe_destroy(void);
void bill_synch(void);
void ben_synch(void);

static atomic_int sandwiches_made;  // rounds of sandwiches Bill has finished
static atomic_int soups_made;       // rounds of soup Ben has finished
static atomic_long violations;
static int rounds;
static int slow_bill, slow_ben;     // milliseconds of extra "cooking" in round 1

static void cook(int id, void *ctx) {
    (void)ctx;
    for (int r = 1; r <= rounds; r++) {
        // make the food
        if (r == 1 && id == 0 && slow_bill) sleep_ms(slow_bill);
        if (r == 1 && id == 1 && slow_ben) sleep_ms(slow_ben);
        harness_jitter();
        if (id == 0) atomic_store(&sandwiches_made, r);
        else atomic_store(&soups_made, r);
        // meet the other cook
        if (id == 0) bill_synch();
        else ben_synch();
        // serve: the other cook's food for round r must be ready now
        int other = id == 0 ? atomic_load(&soups_made) : atomic_load(&sandwiches_made);
        if (other < r) {
            if (atomic_fetch_add(&violations, 1) == 0)
                printf("    %s served round %d, but %s had only finished %d round(s)\n",
                       id == 0 ? "Bill" : "Ben", r, id == 0 ? "Ben's soup" : "Bill's sandwiches",
                       other);
        }
    }
}

static void run(int n_rounds, int bill_delay, int ben_delay) {
    atomic_store(&sandwiches_made, 0);
    atomic_store(&soups_made, 0);
    atomic_store(&violations, 0);
    rounds = n_rounds;
    slow_bill = bill_delay;
    slow_ben = ben_delay;
    cafe_init();
    harness_parallel(2, cook, NULL);
    cafe_destroy();
}

// Ben is slow in round 1, so Bill must wait for him.
static void test_bill_waits_for_slow_ben(void) {
    run(1, 0, 50);
    CHECK(atomic_load(&violations) == 0, "Bill served before Ben's soup was ready");
}

// Bill is slow in round 1, so Ben must wait for him.
static void test_ben_waits_for_slow_bill(void) {
    run(1, 50, 0);
    CHECK(atomic_load(&violations) == 0, "Ben served before Bill's sandwiches were ready");
}

// Many rounds with random pauses: the rule must hold in every round.
static void test_many_rounds(void) {
    long n = harness_iters(20000);
    run((int)n, 0, 0);
    CHECK(atomic_load(&violations) == 0, "the rule was broken %ld times in %ld rounds",
          atomic_load(&violations), n);
    if (atomic_load(&violations) == 0)
        printf("  (no violation observed in %ld rounds - a stress test cannot prove "
               "correctness; the model m1_bill_and_ben.il can)\n", n);
}

int main(void) {
    harness_watchdog(40);
    harness_hint("Each cook should first up() the semaphore that says 'my food is ready', then "
                 "down() the one that says 'the other's food is ready'. Both semaphores start at 0. "
                 "A timeout usually means both cooks wait before either signals.");
    RUN_TEST(test_bill_waits_for_slow_ben);
    RUN_TEST(test_ben_waits_for_slow_bill);
    RUN_TEST(test_many_rounds);
    return harness_report();
}
