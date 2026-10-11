// BUILD: -fopenmp
// REQUIRES: openmp
// TIMEOUT: 60
#include "omp_test.h"

void merge_sort_parallel(int *a, int *tmp, int n, int depth_limit, int nthreads);

static int cmp_int(const void *x, const void *y) {
    int a = *(const int *)x, b = *(const int *)y;
    return (a > b) - (a < b);
}

static void check(int n, int depth_limit, int p, unsigned seed) {
    int m = n > 0 ? n : 1;
    int *a = calloc((size_t)m, sizeof(int)), *want = malloc(sizeof(int) * (size_t)m), *tmp = malloc(sizeof(int) * (size_t)m);
    for (int i = 0; i < n; i++) a[i] = (int)(rand_r(&seed) % 1000);    // many duplicates
    memcpy(want, a, sizeof(int) * (size_t)m);
    qsort(want, (size_t)n, sizeof(int), cmp_int);
    merge_sort_parallel(a, tmp, n, depth_limit, p);
    int sorted = 1, same = memcmp(a, want, sizeof(int) * (size_t)n) == 0;
    for (int i = 1; i < n; i++) sorted &= a[i - 1] <= a[i];
    CHECK(sorted, "n=%d, depth limit %d, %d threads: the result is not sorted", n, depth_limit, p);
    CHECK(same, "n=%d, depth limit %d, %d threads: the result differs from qsort (values lost or duplicated?)",
          n, depth_limit, p);
    free(a); free(want); free(tmp);
}

static void test_tiny(void) { check(0, 3, 4, 1); check(1, 3, 4, 2); check(2, 3, 4, 3); check(17, 2, 3, 4); }
static void test_no_tasks(void) { check(10000, 0, 4, 5); }          // depth limit 0: plain serial sort
static void test_depth_10(void) { check(1000000, 10, 4, 6); check(1000001, 10, 8, 7); }
static void test_tasks_all_the_way_down(void) { check(20000, 30, 4, 8); }   // Lab Q4.2: no cutoff
static void test_repeated(void) {   // a missing taskwait shows up as an unsorted result, sometimes
    long runs = harness_iters(100);
    for (long r = 0; r < runs; r++) check(5000, 6, 8, 100u + (unsigned)r);
}

int main(void) {
    omp_test_skip_under_tsan();
    harness_watchdog(50);
    harness_hint("taskwait must come BEFORE merge (both halves must be finished). Start the "
                 "recursion from inside parallel + single, or every thread sorts the same array.");
    RUN_TEST(test_tiny);
    RUN_TEST(test_no_tasks);
    RUN_TEST(test_depth_10);
    RUN_TEST(test_tasks_all_the_way_down);
    RUN_TEST(test_repeated);
    return harness_report();
}
