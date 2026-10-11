// TIMEOUT: 20
#include "harness.h"

void block_range(long n, int p, int r, long *first, long *count);
int block_owner(long i, long n, int p);
int cyclic_owner(long i, int p);
long cyclic_count(long n, int p, int r);
int block_cyclic_owner(long i, int p, int b);

static void test_block_example(void) {
    long want_first[] = {0, 3, 6, 8}, want_count[] = {3, 3, 2, 2};
    for (int r = 0; r < 4; r++) {
        long f = -1, c = -1;
        block_range(10, 4, r, &f, &c);
        CHECK(f == want_first[r] && c == want_count[r],
              "n=10, p=4, rank %d: got first=%ld count=%ld, expected first=%ld count=%ld",
              r, f, c, want_first[r], want_count[r]);
    }
}

static void test_block_all(void) {
    int bad = 0;
    for (long n = 0; n <= 60 && !bad; n++)
        for (int p = 1; p <= 9 && !bad; p++) {
            long next = 0, minc = n, maxc = 0;
            for (int r = 0; r < p; r++) {
                long f = -1, c = -1;
                block_range(n, p, r, &f, &c);
                if (f != next) { CHECK(0, "n=%ld p=%d rank %d starts at %ld, expected %ld (ranges must follow each other with no gap)", n, p, r, f, next); bad = 1; break; }
                if (c < minc) minc = c;
                if (c > maxc) maxc = c;
                for (long i = f; i < f + c; i++)
                    if (block_owner(i, n, p) != r) { CHECK(0, "n=%ld p=%d: block_owner(%ld) = %d, but block_range gives it to rank %d", n, p, i, block_owner(i, n, p), r); bad = 1; break; }
                next = f + c;
            }
            if (!bad && next != n) { CHECK(0, "n=%ld p=%d: the ranges cover %ld elements, expected %ld", n, p, next, n); bad = 1; }
            if (!bad && maxc - minc > 1) { CHECK(0, "n=%ld p=%d: counts differ by %ld; they must differ by at most 1", n, p, maxc - minc); bad = 1; }
        }
    CHECK(!bad, "block partitioning must be correct for every n <= 60 and p <= 9");
}

static void test_cyclic(void) {
    int want[] = {0, 1, 2, 3, 0, 1, 2, 3, 0, 1};
    for (int i = 0; i < 10; i++) CHECK_EQ(cyclic_owner(i, 4), want[i]);
    int bad = 0;
    for (long n = 0; n <= 60; n++)
        for (int p = 1; p <= 9; p++)
            for (int r = 0; r < p; r++) {
                long c = 0;
                for (long i = 0; i < n; i++) c += cyclic_owner(i, p) == r;
                if (c != cyclic_count(n, p, r)) bad++;
            }
    CHECK(bad == 0, "cyclic_count disagrees with cyclic_owner in %d cases", bad);
}

static void test_block_cyclic(void) {
    int want[] = {0, 0, 1, 1, 2, 2, 0, 0, 1, 1, 2, 2};
    for (int i = 0; i < 12; i++) CHECK_EQ(block_cyclic_owner(i, 3, 2), want[i]);
    for (int i = 0; i < 12; i++) CHECK_EQ(block_cyclic_owner(i, 4, 1), i % 4);   // b = 1 is plain cyclic
}

int main(void) {
    harness_watchdog(10);
    harness_hint("block: q = n / p, extra = n % p; rank r gets q + (r < extra), first = r*q + min(r, extra)");
    RUN_TEST(test_block_example);
    RUN_TEST(test_block_all);
    RUN_TEST(test_cyclic);
    RUN_TEST(test_block_cyclic);
    return harness_report();
}
