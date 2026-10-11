// TIMEOUT: 20
#include "banker_safety_tests.h"

static int max5[MAXP][MAXR] = {{5, 2, 2}, {3, 2, 3}, {5, 3, 3}, {6, 1, 2}, {2, 4, 2}};
static int avail5[MAXR];
static int alloc5[MAXP][MAXR];
static void reset5(void) {
    int a[MAXR] = {3, 1, 2};
    int c[MAXP][MAXR] = {{1, 1, 0}, {2, 0, 1}, {0, 2, 2}, {3, 0, 1}, {1, 1, 1}};
    memcpy(avail5, a, sizeof a);
    memcpy(alloc5, c, sizeof c);
}
static bool unchanged5(void) {
    int a[MAXR] = {3, 1, 2};
    int c[MAXP][MAXR] = {{1, 1, 0}, {2, 0, 1}, {0, 2, 2}, {3, 0, 1}, {1, 1, 1}};
    return memcmp(avail5, a, sizeof(int) * 3) == 0 && memcmp(alloc5, c, sizeof c) == 0;
}

// P4 asks for (1 0 0): afterwards P4 needs (2 1 1) and still finishes first. Grant.
static void test_request_granted(void) {
    reset5();
    int req[MAXR] = {1, 0, 0};
    CHECK(request_ok(5, 3, avail5, max5, alloc5, 3, req), "P4's request (1 0 0) is safe: grant it");
    CHECK(avail5[0] == 2 && alloc5[3][0] == 4,
          "after granting, Available R1 should be 2 and P4 should hold 4 R1 (got %d and %d)",
          avail5[0], alloc5[3][0]);
}

// P5 asks for (0 1 0): it is free, but afterwards no R2 is left and nobody
// can surely finish. Refuse, and leave everything unchanged.
static void test_request_unsafe_must_wait(void) {
    reset5();
    int req[MAXR] = {0, 1, 0};
    CHECK(!request_ok(5, 3, avail5, max5, alloc5, 4, req),
          "P5's request (0 1 0) leads to an unsafe state: it must wait even though it is free");
    CHECK(unchanged5(), "a refused request must leave Available and Alloc exactly as before");
}

static void test_request_more_than_free(void) {
    reset5();
    int req[MAXR] = {0, 2, 0};                    // P5 may need 3 more R2, but only 1 is free
    CHECK(!request_ok(5, 3, avail5, max5, alloc5, 4, req), "only 1 R2 is free: P5 must wait");
    CHECK(unchanged5(), "a refused request must leave Available and Alloc exactly as before");
}

static void test_request_more_than_declared(void) {
    reset5();
    int req[MAXR] = {2, 0, 0};                    // P2 needs at most 1 more R1
    CHECK(!request_ok(5, 3, avail5, max5, alloc5, 1, req),
          "P2 declared Max R1 = 3 and holds 2, so asking for 2 more breaks its promise");
    CHECK(unchanged5(), "a refused request must leave Available and Alloc exactly as before");
}

// One resource, 10 units: A 3/9, B 2/4, C 2/7. A asks for 1: unsafe.
static void test_one_resource_request(void) {
    int avail[MAXR] = {3};
    int max[MAXP][MAXR] = {{9}, {4}, {7}};
    int alloc[MAXP][MAXR] = {{3}, {2}, {2}};
    int one[MAXR] = {1};
    CHECK(!request_ok(3, 1, avail, max, alloc, 0, one), "A asking for 1 more makes the state unsafe");
    CHECK(avail[0] == 3 && alloc[0][0] == 3, "a refused request must change nothing");
    CHECK(request_ok(3, 1, avail, max, alloc, 1, one), "B asking for 1 more is safe: grant it");
}

int main(void) {
    run_safety_tests();
    RUN_TEST(test_request_granted);
    RUN_TEST(test_request_unsafe_must_wait);
    RUN_TEST(test_request_more_than_free);
    RUN_TEST(test_request_more_than_declared);
    RUN_TEST(test_one_resource_request);
    return harness_report();
}
