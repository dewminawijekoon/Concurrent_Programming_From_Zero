// TIMEOUT: 30
#include "harness.h"

int create_and_join(int n, long *results);

static void check_n(int n) {
    long results[64];
    for (int i = 0; i < 64; i++) results[i] = -1;    // "nothing returned yet"
    int rc = create_and_join(n, results);
    CHECK(rc == 0, "create_and_join(%d, ...) returned %d, expected 0", n, rc);
    int bad = 0;
    for (int i = 0; i < n; i++)
        if (results[i] != (long)i * i) {
            if (bad == 0)
                printf("    results[%d] = %ld, but thread %d should return %d\n", i, results[i],
                       i, i * i);
            bad++;
        }
    CHECK(bad == 0, "with %d threads, %d results were wrong", n, bad);
    CHECK(results[n] == -1, "with %d threads, results[%d] was written (only 0..%d exist)", n, n,
          n - 1);
}

// The lab asks for N = 1, 4, 8 and 16.
static void test_one_thread(void) { check_n(1); }
static void test_four_threads(void) { check_n(4); }
static void test_eight_threads(void) { check_n(8); }
static void test_sixteen_threads(void) { check_n(16); }

int main(void) {
    harness_watchdog(20);
    harness_hint("Pass the index as (void *)(intptr_t)i, return (void *)(intptr_t)(i*i), and read "
                 "it with pthread_join(tid[i], &ret). Remember to return 0.");
    RUN_TEST(test_one_thread);
    RUN_TEST(test_four_threads);
    RUN_TEST(test_eight_threads);
    RUN_TEST(test_sixteen_threads);
    return harness_report();
}
