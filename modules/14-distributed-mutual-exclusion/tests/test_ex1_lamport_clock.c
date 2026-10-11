// TIMEOUT: 20
#include "harness.h"

int lc_local(int *clock);
int lc_send(int *clock);
int lc_receive(int *clock, int msg_ts);
int lc_before(int ts_a, int id_a, int ts_b, int id_b);
typedef struct { int proc; char kind; int msg; } lc_event;
void lamport_run(const lc_event *ev, int n, int nprocs, int *out_ts);

static void test_rules(void) {
    int c = 0;
    CHECK_EQ(lc_local(&c), 1);
    CHECK_EQ(lc_send(&c), 2);
    CHECK_EQ(c, 2);
    CHECK_EQ(lc_receive(&c, 7), 8);    // jump past the sender's clock
    CHECK_EQ(lc_receive(&c, 3), 9);    // an old message still moves the clock forward by 1
    CHECK_EQ(c, 9);
}

static void test_total_order(void) {
    CHECK(lc_before(3, 2, 5, 0), "timestamp 3 comes before timestamp 5, whatever the ids");
    CHECK(!lc_before(5, 0, 3, 2), "timestamp 5 does not come before timestamp 3");
    CHECK(lc_before(4, 1, 4, 2), "equal timestamps: the smaller process id goes first");
    CHECK(!lc_before(4, 2, 4, 1), "equal timestamps: the larger process id goes second");
    CHECK(!lc_before(4, 1, 4, 1), "an event is not before itself");
}

static void test_readme_example(void) {
    lc_event ev[] = {
        {0, 'L', 0}, {1, 'L', 0}, {2, 'L', 0}, {0, 'S', 1}, {1, 'R', 1},
        {1, 'S', 2}, {2, 'L', 0}, {2, 'R', 2}, {2, 'S', 3}, {0, 'R', 3},
    };
    int want[] = {1, 1, 1, 2, 3, 4, 2, 5, 6, 7};
    int ts[10] = {0};
    lamport_run(ev, 10, 3, ts);
    for (int e = 0; e < 10; e++) CHECK(ts[e] == want[e], "event %d (P%d, %c): timestamp %d, expected %d", e, ev[e].proc, ev[e].kind, ts[e], want[e]);
}

static void test_happened_before(void) {
    // A chain P0 -> P1 -> P2 -> P3 with local events in between, plus a busy P3.
    lc_event ev[] = {
        {3, 'L', 0}, {3, 'L', 0}, {3, 'L', 0}, {3, 'L', 0}, {3, 'L', 0},
        {0, 'S', 1}, {1, 'L', 0}, {1, 'R', 1}, {1, 'S', 2}, {2, 'R', 2},
        {2, 'S', 3}, {3, 'R', 3}, {3, 'L', 0},
    };
    int ts[13] = {0};
    lamport_run(ev, 13, 4, ts);
    int want[] = {1, 2, 3, 4, 5, 1, 1, 2, 3, 4, 5, 6, 7};
    for (int e = 0; e < 13; e++) CHECK(ts[e] == want[e], "chain event %d: timestamp %d, expected %d", e, ts[e], want[e]);
    // every send is before its receive
    CHECK(ts[5] < ts[7] && ts[8] < ts[9] && ts[10] < ts[11], "a send must always have a smaller timestamp than its receive");
}

int main(void) {
    harness_watchdog(10);
    harness_hint("local and send: clock + 1; receive: max(clock, message) + 1; ties broken by the smaller id");
    RUN_TEST(test_rules);
    RUN_TEST(test_total_order);
    RUN_TEST(test_readme_example);
    RUN_TEST(test_happened_before);
    return harness_report();
}
