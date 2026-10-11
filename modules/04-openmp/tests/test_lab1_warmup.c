// BUILD: -fopenmp
// REQUIRES: openmp
// TIMEOUT: 30
#include "omp_test.h"

int hello_team(int nthreads, int *seen);
void firstprivate_demo(int x, int nthreads, int *start_value, int *after);
void squares_static(int *arr, int *owner, int n, int nthreads);

static void test_task_a_hello(void) {
    for (int p = 1; p <= 8; p *= 2) {
        int seen[64] = {0};
        int team = hello_team(p, seen);
        CHECK(team == p, "hello_team(%d): team size %d", p, team);
        int count = 0;
        for (int i = 0; i < 64; i++) count += seen[i];
        CHECK(count == team && team > 0, "hello_team(%d): %d threads marked seen[], team is %d", p, count, team);
    }
}

static void test_task_b_firstprivate(void) {
    int start[64], after = 0;
    for (int i = 0; i < 64; i++) start[i] = -999;
    firstprivate_demo(100, 4, start, &after);
    for (int t = 0; t < 4; t++)
        CHECK(start[t] == 100, "thread %d's copy of x started at %d, expected 100 (firstprivate copies the outside value)", t, start[t]);
    CHECK(after == 100, "after the region x = %d, expected 100 (only private copies changed)", after);
}

// The OpenMP standard only says that schedule(static) without a chunk size
// gives each thread at most one block of roughly equal size, in thread
// order. GCC makes the first n % p blocks one bigger. We check the
// standard's promise, plus the exact GCC split for 20 and 10 iterations.
static void check_static(int n, int p) {
    int arr[256], owner[256];
    for (int i = 0; i < n; i++) arr[i] = owner[i] = -1;
    squares_static(arr, owner, n, p);
    int bad = 0;
    for (int i = 0; i < n; i++) bad += arr[i] != i * i;
    CHECK(bad == 0, "n=%d: %d entries of arr[] are not i*i", n, bad);
    int ok = owner[0] == 0;
    for (int i = 1; i < n; i++) ok &= owner[i] == owner[i - 1] || owner[i] == owner[i - 1] + 1;
    CHECK(ok, "n=%d, %d threads: owner[] should be one block per thread, in thread order (0 0 0 1 1 ...)", n, p);
    int size[64] = {0};
    for (int i = 0; i < n; i++) if (owner[i] >= 0 && owner[i] < 64) size[owner[i]]++;
    for (int t = 0; t < p; t++) {
        int want = n / p + (t < n % p);
        CHECK(size[t] == want, "n=%d, %d threads: thread %d computed %d indices, expected %d", n, p, t, size[t], want);
    }
}
static void test_task_c_static_mapping(void) { check_static(20, 4); check_static(10, 4); check_static(7, 3); }

int main(void) {
    omp_test_skip_under_tsan();
    harness_watchdog(20);
    harness_hint("Task A: single inside the region. Task B: firstprivate(x). Task C: "
                 "parallel for schedule(static) num_threads(nthreads), owner[i] = omp_get_thread_num().");
    RUN_TEST(test_task_a_hello);
    RUN_TEST(test_task_b_firstprivate);
    RUN_TEST(test_task_c_static_mapping);
    return harness_report();
}
