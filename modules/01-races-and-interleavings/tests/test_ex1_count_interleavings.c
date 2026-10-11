// TIMEOUT: 30
#include <string.h>

#include "harness.h"
#include "ex1_count_interleavings.h"

static char out[4000][LIST_LINE];

// Every string must have m1 'A's and m2 'B's, and all strings must differ.
static void check_list(int m1, int m2, int expected) {
    memset(out, 0, sizeof out);
    int c = list_interleavings(m1, m2, out, 4000);
    CHECK(c == expected, "list_interleavings(%d, %d) returned %d, expected %d", m1, m2, c, expected);
    if (c != expected) return;
    for (int i = 0; i < c; i++) {
        int a = 0, b = 0, other = 0;
        for (const char *p = out[i]; *p; p++) {
            if (*p == 'A') a++;
            else if (*p == 'B') b++;
            else other++;
        }
        if (a != m1 || b != m2 || other) {
            CHECK(0, "out[%d] = \"%s\" should have %d A's and %d B's and nothing else", i, out[i], m1, m2);
            return;
        }
        for (int k = 0; k < i; k++)
            if (strcmp(out[i], out[k]) == 0) {
                CHECK(0, "out[%d] and out[%d] are both \"%s\": each interleaving must appear once", k, i, out[i]);
                return;
            }
    }
}

static void test_list_small(void) {
    check_list(1, 1, 2);    // AB, BA
    check_list(2, 2, 6);    // the six orders on L03 slide 33
    check_list(3, 2, 10);
    check_list(0, 3, 1);    // only one thread: just one order
}

static void test_list_respects_max(void) {
    int c = list_interleavings(5, 5, out, 3);
    CHECK(c == 252, "list_interleavings(5, 5, out, max=3) should still return the full count 252, got %d", c);
}

static void test_count_lecture_example(void) {
    CHECK_EQ(count_interleavings(2, 2), 6);
    CHECK_EQ(count_interleavings(3, 2), 90);   // L03 slide 34
}

static void test_count_more(void) {
    CHECK(count_interleavings(2, 3) == 20ULL, "count_interleavings(2, 3) = %llu, expected 20", count_interleavings(2, 3));
    CHECK(count_interleavings(3, 3) == 1680ULL, "count_interleavings(3, 3) = %llu, expected 1680", count_interleavings(3, 3));
    CHECK(count_interleavings(4, 5) == 11732745024ULL, "count_interleavings(4, 5) = %llu, expected 11732745024", count_interleavings(4, 5));
    CHECK(count_interleavings(5, 5) == 623360743125120ULL, "count_interleavings(5, 5) = %llu, expected 623360743125120", count_interleavings(5, 5));
    CHECK(count_interleavings(2, 33) == 7219428434016265740ULL, "count_interleavings(2, 33) = %llu, expected 7219428434016265740", count_interleavings(2, 33));
    CHECK(count_interleavings(1, 7) == 1ULL, "one thread has exactly 1 order, got %llu", count_interleavings(1, 7));
}

static void test_count_mixed_lengths(void) {
    int len[3] = {1, 2, 3};
    CHECK(count_interleavings_lengths(3, len) == 60ULL,
          "lengths {1,2,3}: got %llu, expected 6!/(1!*2!*3!) = 60", count_interleavings_lengths(3, len));
    int len2[2] = {3, 2};
    CHECK(count_interleavings_lengths(2, len2) == 10ULL,
          "lengths {3,2}: got %llu, expected 10", count_interleavings_lengths(2, len2));
}

static void test_count_too_big_returns_zero(void) {
    CHECK(count_interleavings(2, 34) == 0ULL,
          "count_interleavings(2, 34) is about 2.8e19, too big for 64 bits: expected 0, got %llu",
          count_interleavings(2, 34));
    CHECK(count_interleavings(6, 6) == 0ULL,
          "count_interleavings(6, 6) is about 2.7e24: expected 0, got %llu", count_interleavings(6, 6));
    CHECK(count_interleavings(10, 10) == 0ULL,
          "count_interleavings(10, 10) is about 2.4e92: expected 0, got %llu", count_interleavings(10, 10));
}

int main(void) {
    harness_watchdog(20);
    harness_hint("Part A: recursion on (A steps left, B steps left). Part B: multiply "
                 "binomial(free slots, len[j]) for each thread and subtract the slots it used.");
    RUN_TEST(test_list_small);
    RUN_TEST(test_list_respects_max);
    RUN_TEST(test_count_lecture_example);
    RUN_TEST(test_count_more);
    RUN_TEST(test_count_mixed_lengths);
    RUN_TEST(test_count_too_big_returns_zero);
    return harness_report();
}
