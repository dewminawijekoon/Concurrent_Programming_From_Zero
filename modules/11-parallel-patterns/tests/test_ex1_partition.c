// TIMEOUT: 30
#include "harness.h"

void block_range(long n, int p, int r, long *lo, long *hi);
int block_owner(long n, int p, long i);
int cyclic_owner(int p, long i);
long cyclic_count(long n, int p, int r);
int block_cyclic_owner(int p, long b, long i);

// The example from the exercise header: 10 items, 4 threads.
static void test_block_example(void) {
    long want[4][2] = {{0, 3}, {3, 6}, {6, 8}, {8, 10}};
    for (int r = 0; r < 4; r++) {
        long lo = -1, hi = -1;
        block_range(10, 4, r, &lo, &hi);
        CHECK(lo == want[r][0] && hi == want[r][1],
              "n=10 p=4: thread %d got [%ld, %ld), expected [%ld, %ld)", r, lo, hi, want[r][0],
              want[r][1]);
    }
}

// Every n and p up to a limit: the blocks must cover 0..n-1 in order, with no
// gaps and no overlap, and the first n % p blocks are one item bigger.
static void test_block_all_sizes(void) {
    int bad = 0;
    for (long n = 0; n <= 70 && bad < 5; n++)
        for (int p = 1; p <= 9 && bad < 5; p++) {
            long expect_lo = 0;
            for (int r = 0; r < p; r++) {
                long lo = -1, hi = -1;
                block_range(n, p, r, &lo, &hi);
                long size = n / p + (r < n % p ? 1 : 0);
                if (lo != expect_lo || hi - lo != size) {
                    CHECK(0, "n=%ld p=%d thread %d: got [%ld, %ld), expected [%ld, %ld)", n, p, r,
                          lo, hi, expect_lo, expect_lo + size);
                    bad++;
                    break;
                }
                for (long i = lo; i < hi; i++)
                    if (block_owner(n, p, i) != r) {
                        CHECK(0, "n=%ld p=%d: block_owner(item %ld) = %d, but block_range gives it "
                                 "to thread %d", n, p, i, block_owner(n, p, i), r);
                        bad++;
                        break;
                    }
                expect_lo = hi;
            }
        }
    CHECK(bad == 0, "block partitioning is wrong for some sizes (see above)");
}

static void test_cyclic(void) {
    int bad = 0;
    for (long n = 0; n <= 50 && bad < 5; n++)
        for (int p = 1; p <= 7 && bad < 5; p++) {
            long count[8] = {0};
            for (long i = 0; i < n; i++) {
                int o = cyclic_owner(p, i);
                if (o != (int)(i % p)) {
                    CHECK(0, "p=%d: cyclic_owner(item %ld) = %d, expected %d", p, i, o,
                          (int)(i % p));
                    bad++;
                    break;
                }
                count[o]++;
            }
            for (int r = 0; r < p && bad < 5; r++)
                if (cyclic_count(n, p, r) != count[r]) {
                    CHECK(0, "n=%ld p=%d: cyclic_count(thread %d) = %ld, expected %ld", n, p, r,
                          cyclic_count(n, p, r), count[r]);
                    bad++;
                }
        }
    CHECK(bad == 0, "cyclic partitioning is wrong for some sizes (see above)");
}

static void test_block_cyclic(void) {
    // 12 items, 2 threads, blocks of 2: 0 0 1 1 0 0 1 1 0 0 1 1
    int want[12] = {0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1};
    for (long i = 0; i < 12; i++)
        CHECK(block_cyclic_owner(2, 2, i) == want[i], "p=2 b=2: item %ld -> %d, expected %d", i,
              block_cyclic_owner(2, 2, i), want[i]);
    // block size 1 is the same as cyclic
    for (long i = 0; i < 30; i++)
        CHECK(block_cyclic_owner(4, 1, i) == (int)(i % 4), "b=1 must equal cyclic (item %ld)", i);
    // 3 threads, blocks of 4: items 0-3 -> 0, 4-7 -> 1, 8-11 -> 2, 12-15 -> 0
    CHECK_EQ(block_cyclic_owner(3, 4, 3), 0);
    CHECK_EQ(block_cyclic_owner(3, 4, 4), 1);
    CHECK_EQ(block_cyclic_owner(3, 4, 11), 2);
    CHECK_EQ(block_cyclic_owner(3, 4, 12), 0);
}

int main(void) {
    harness_watchdog(20);
    harness_hint("Work out n=10, p=4 by hand first: q = n / p = 2 and rem = n % p = 2, so threads "
                 "0 and 1 get 3 items and threads 2 and 3 get 2 items.");
    RUN_TEST(test_block_example);
    RUN_TEST(test_block_all_sizes);
    RUN_TEST(test_cyclic);
    RUN_TEST(test_block_cyclic);
    return harness_report();
}
