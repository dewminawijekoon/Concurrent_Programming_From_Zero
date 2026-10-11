// TIMEOUT: 20
#include "harness.h"
#include "ra.h"

int ra_earlier(int ts_a, int id_a, int ts_b, int id_b);
int ra_should_defer(int my_state, int my_ts, int my_id, int req_ts, int req_id);

void net_send(const ra_msg *m) { (void)m; }   // not used here

static void test_earlier(void) {
    CHECK(ra_earlier(3, 5, 4, 0) == 1, "ts 3 is earlier than ts 4");
    CHECK(ra_earlier(4, 0, 3, 5) == 0, "ts 4 is not earlier than ts 3");
    CHECK(ra_earlier(5, 1, 5, 2) == 1, "equal ts: id 1 is earlier than id 2");
    CHECK(ra_earlier(5, 2, 5, 1) == 0, "equal ts: id 2 is not earlier than id 1");
}

static void test_cases(void) {
    // RELEASED: always reply at once
    CHECK(ra_should_defer(RA_RELEASED, 0, 0, 9, 3) == 0, "RELEASED: must reply now");
    CHECK(ra_should_defer(RA_RELEASED, 1, 0, 9, 3) == 0, "RELEASED (old my_ts left over): must still reply now");
    // HELD: always defer
    CHECK(ra_should_defer(RA_HELD, 9, 3, 1, 0) == 1, "HELD: must defer, even for an earlier request");
    // WANTED: the earlier request wins
    CHECK(ra_should_defer(RA_WANTED, 4, 2, 7, 1) == 1, "WANTED with ts 4 vs request ts 7: mine is earlier, defer");
    CHECK(ra_should_defer(RA_WANTED, 7, 1, 4, 2) == 0, "WANTED with ts 7 vs request ts 4: theirs is earlier, reply now");
    CHECK(ra_should_defer(RA_WANTED, 5, 1, 5, 2) == 1, "WANTED, equal ts 5: my id 1 < 2, defer");
    CHECK(ra_should_defer(RA_WANTED, 5, 2, 5, 1) == 0, "WANTED, equal ts 5: my id 2 > 1, reply now");
}

static void test_symmetry(void) {
    // For any two competing requests, exactly one of the two nodes defers.
    int bad = 0;
    for (int ta = 1; ta <= 6; ta++)
        for (int tb = 1; tb <= 6; tb++)
            for (int a = 0; a < 4; a++)
                for (int b = 0; b < 4; b++) {
                    if (a == b) continue;
                    int da = ra_should_defer(RA_WANTED, ta, a, tb, b);
                    int db = ra_should_defer(RA_WANTED, tb, b, ta, a);
                    if (da + db != 1) bad++;
                }
    CHECK(bad == 0, "%d pairs of competing requests where not exactly one node defers (both defer = deadlock, none = both enter)", bad);
}

int main(void) {
    harness_watchdog(10);
    harness_hint("defer if HELD, or if WANTED and my (ts, id) is earlier than theirs; otherwise reply now");
    RUN_TEST(test_earlier);
    RUN_TEST(test_cases);
    RUN_TEST(test_symmetry);
    return harness_report();
}
