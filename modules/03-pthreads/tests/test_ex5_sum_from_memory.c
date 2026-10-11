// TIMEOUT: 30
#include "harness.h"

long threaded_sum(const long *a, long n, int nthreads);

static void check_case(long n, int p) {
    long *a = malloc(sizeof *a * (size_t)(n > 0 ? n : 1)), want = 0;
    for (long i = 0; i < n; i++) {
        a[i] = (i * 7919) % 1000 - 300;          // a mix of positive and negative numbers
        want += a[i];
    }
    long got = threaded_sum(a, n, p);
    CHECK(got == want, "n=%ld, %d threads: got %ld, expected %ld", n, p, got, want);
    free(a);
}

static void test_small(void) { check_case(10, 2); check_case(7, 3); }
static void test_more_threads_than_items(void) { check_case(3, 8); check_case(1, 4); }
static void test_empty_array(void) { check_case(0, 4); check_case(5, 1); }
static void test_large(void) { check_case(3000001, 8); }
static void test_repeated(void) {                // a missing lock shows up as a wrong sum
    long runs = harness_iters(200);
    for (long r = 0; r < runs; r++) check_case(1000, 16);
}

int main(void) {
    harness_watchdog(20);
    harness_hint("Pattern: struct per thread -> pthread_create(&tid[t], NULL, worker, &args[t]) "
                 "-> local sum -> lock, total += local, unlock -> pthread_join all -> return total.");
    RUN_TEST(test_small);
    RUN_TEST(test_more_threads_than_items);
    RUN_TEST(test_empty_array);
    RUN_TEST(test_large);
    RUN_TEST(test_repeated);
    return harness_report();
}
