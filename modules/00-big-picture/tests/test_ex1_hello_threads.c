// TIMEOUT: 30
#include "harness.h"
#include "ex1_hello_threads.h"

static void run_with(int n) {
    struct hello_slot slots[HELLO_MAX];
    pthread_t me = pthread_self();
    for (int i = 0; i < HELLO_MAX; i++) {     // fill with "nothing written yet" markers
        slots[i].index = -1;
        slots[i].square = -1;
        slots[i].who = me;
    }
    int r = hello_threads(n, slots);
    CHECK(r == n, "hello_threads(%d) returned %d, expected %d", n, r, n);

    int missing = 0, bad_square = 0, ran_in_main = 0, same_thread = 0;
    for (int i = 0; i < n; i++) {
        if (slots[i].index != i) missing++;
        if (slots[i].square != (long)i * i) bad_square++;
        if (pthread_equal(slots[i].who, me)) ran_in_main++;
        for (int k = 0; k < i; k++)
            if (pthread_equal(slots[i].who, slots[k].who)) same_thread++;
    }
    CHECK(missing == 0, "n=%d: %d slots do not hold their own number (slots[i].index should be i)",
          n, missing);
    CHECK(bad_square == 0,
          "n=%d: %d slots have the wrong square (thread i must return i*i, and main stores it)",
          n, bad_square);
    CHECK(ran_in_main == 0,
          "n=%d: %d slots were filled by the main thread, not by a new thread", n, ran_in_main);
    if (ran_in_main == 0)
        CHECK(same_thread == 0,
              "n=%d: %d pairs of slots share a thread id. Start ALL threads first, then join "
              "them (a joined thread's id can be reused)", n, same_thread);
}

static void test_one_thread(void) { run_with(1); }
static void test_four_threads(void) { run_with(4); }
static void test_eight_threads(void) { run_with(8); }
static void test_sixteen_threads(void) { run_with(16); }

int main(void) {
    harness_watchdog(20);
    harness_hint("Pass i as (void *)(long)i, read it back with (int)(long)arg, "
                 "and collect the result with pthread_join(t[i], &result).");
    RUN_TEST(test_one_thread);
    RUN_TEST(test_four_threads);
    RUN_TEST(test_eight_threads);
    RUN_TEST(test_sixteen_threads);
    return harness_report();
}
