// TIMEOUT: 60
// Checks lab1_ping_pong: the words must be exactly ping pong ping pong ...
#include "harness.h"
#include "trace.h"

#define PING 0
#define PONG 1
void pingpong(int n);
extern void (*pingpong_say)(int who, int round);

// Our replacement for the printing function: log who spoke, and in which round.
static void record(int who, int round) {
    trace_event(who, who == PING ? "ping" : "pong", round);
    harness_jitter();   // sometimes give up the CPU, to try more orders
}

static int check_rounds(int n) {
    trace_reset();
    pingpong(n);
    if (trace_len != 2L * n) {
        CHECK(0, "n = %d: expected %d words (n pings + n pongs), got %ld", n, 2 * n, trace_len);
        trace_dump_tail(10);
        return 0;
    }
    for (long k = 0; k < trace_len; k++) {
        int want_who = (int)(k % 2);    // even positions: ping, odd positions: pong
        long want_round = k / 2;
        if (trace_log[k].thread != want_who || trace_log[k].value != want_round) {
            CHECK(0, "n = %d: word number %ld should be \"%s\" (round %ld), but it was \"%s\" (round %ld)",
                  n, k + 1, want_who == PING ? "ping" : "pong", want_round,
                  trace_log[k].action, trace_log[k].value);
            long from = k > 5 ? k - 5 : 0;
            printf("    words %ld to %ld:", from + 1, k + 1);
            for (long q = from; q <= k; q++) printf(" %s", trace_log[q].action);
            printf("\n");
            return 0;
        }
    }
    return 1;
}

static void test_small_rounds(void) {
    int ns[] = {1, 2, 5, 10, 20};
    for (int i = 0; i < 5; i++)
        if (!check_rounds(ns[i])) return;
    CHECK(1, "ok");
}

static void test_many_rounds(void) {
    int n = (int)harness_iters(20000);
    if (check_rounds(n)) printf("  (%d rounds alternated perfectly in this run)\n", n);
}

int main(void) {
    harness_watchdog(40);
    harness_hint("Each player must wait in a while loop until turn == me, then speak, "
                 "set turn to the other player and signal, all while holding the mutex.");
    pingpong_say = record;
    RUN_TEST(test_small_rounds);
    RUN_TEST(test_many_rounds);
    return harness_report();
}
