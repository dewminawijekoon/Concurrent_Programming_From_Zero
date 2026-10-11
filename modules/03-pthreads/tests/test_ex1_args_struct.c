// TIMEOUT: 30
#include "harness.h"
#include "ex1_args_struct.h"

static void check_case(long n, int p) {
    struct block_arg *args = malloc(sizeof *args * (size_t)p);
    for (int t = 0; t < p; t++)       // garbage, so untouched fields are noticed
        args[t] = (struct block_arg){-1, -1, -1, -1, -1, -1};
    long total = sum_in_blocks(n, p, args);
    CHECK(total == n * (n - 1) / 2, "n=%ld, %d threads: total is %ld, expected %ld", n, p,
          total, n * (n - 1) / 2);
    for (int t = 0; t < p; t++) {
        long f = (long)t * n / p, l = (long)(t + 1) * n / p, s = 0;
        for (long i = f; i < l; i++) s += i;
        CHECK(args[t].id == t && args[t].nthreads == p && args[t].n == n,
              "n=%ld, %d threads: the 'in' fields of args[%d] were not filled in", n, p, t);
        CHECK(args[t].first == f && args[t].last == l,
              "n=%ld, %d threads: thread %d handled [%ld,%ld), expected [%ld,%ld)", n, p, t,
              args[t].first, args[t].last, f, l);
        CHECK(args[t].partial == s, "n=%ld, %d threads: thread %d partial sum %ld, expected %ld",
              n, p, t, args[t].partial, s);
    }
    free(args);
}

static void test_divides_evenly(void) { check_case(12, 3); check_case(1000, 4); }
static void test_does_not_divide(void) { check_case(10, 3); check_case(1001, 7); }
static void test_more_threads_than_numbers(void) { check_case(3, 8); }
static void test_many_threads(void) { check_case(5000000, 16); }

int main(void) {
    harness_watchdog(20);
    harness_hint("Fill args[t] before pthread_create, pass &args[t], and add the partial sums "
                 "only AFTER pthread_join (before that the thread may not have finished).");
    RUN_TEST(test_divides_evenly);
    RUN_TEST(test_does_not_divide);
    RUN_TEST(test_more_threads_than_numbers);
    RUN_TEST(test_many_threads);
    return harness_report();
}
