// TIMEOUT: 60
#include "harness.h"
#include "ra.h"

void ra_request_cs(ra_node *me);
void ra_on_message(ra_node *me, const ra_msg *m);
void ra_release_cs(ra_node *me);

#define POOL 4096
static ra_msg pool[POOL];
static int npool, sent, bad_msg;
static unsigned rng;

void net_send(const ra_msg *m) {
    if (npool >= POOL || m->to < 0 || m->to >= RA_MAXN || m->from < 0 || m->from >= RA_MAXN || m->to == m->from) { bad_msg++; return; }
    pool[npool++] = *m;
    sent++;
}
static unsigned rnd(void) { rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5; return rng; }

// One random run: n nodes, each enters k times. Returns 1 if fine.
static int run(int n, int k, unsigned seed, long *msgs_out) {
    ra_node node[RA_MAXN];
    int left[RA_MAXN], inside_for[RA_MAXN];
    memset(node, 0, sizeof node);
    for (int i = 0; i < n; i++) { node[i].id = i; node[i].n = n; left[i] = k; inside_for[i] = 0; }
    npool = 0; sent = 0; bad_msg = 0; rng = seed * 2654435761u + 1;
    long steps = 0, total_entries = 0;
    while (total_entries < (long)n * k) {
        if (++steps > 200000) { CHECK(0, "n=%d seed=%u: no progress after 200000 steps (deadlock or starvation)", n, seed); return 0; }
        unsigned choice = rnd() % 3;
        int i = (int)(rnd() % n);
        if (choice == 0 && node[i].state == RA_RELEASED && left[i] > 0) {
            ra_request_cs(&node[i]);
        } else if (choice == 1 && npool > 0) {
            int j = (int)(rnd() % npool);
            ra_msg m = pool[j];
            pool[j] = pool[--npool];                 // deliver in random order
            ra_on_message(&node[m.to], &m);
        } else if (choice == 2 && node[i].state == RA_HELD) {
            if (++inside_for[i] >= 2) {              // stay inside for a little while
                inside_for[i] = 0;
                left[i]--;
                total_entries++;
                ra_release_cs(&node[i]);
            }
        }
        if (bad_msg) { CHECK(0, "n=%d: a message with a wrong sender/receiver id (or to itself) was sent", n); return 0; }
        int held = 0;
        for (int x = 0; x < n; x++) held += node[x].state == RA_HELD;
        if (held > 1) { CHECK(0, "n=%d seed=%u step %ld: %d nodes are HELD at the same time (mutual exclusion broken)", n, seed, steps, held); return 0; }
        // a node that is WANTED with nothing in flight and nobody else able to move = stuck
        if (npool == 0) {
            int movable = 0;
            for (int x = 0; x < n; x++) movable |= node[x].state == RA_HELD || (node[x].state == RA_RELEASED && left[x] > 0);
            if (!movable && total_entries < (long)n * k) {
                CHECK(0, "n=%d seed=%u: some node waits for replies that will never come (deadlock); a deferred reply was never sent?", n, seed);
                return 0;
            }
        }
    }
    // let the last messages arrive
    while (npool > 0) { ra_msg m = pool[--npool]; ra_on_message(&node[m.to], &m); }
    *msgs_out = sent;
    return 1;
}

static void test_many_runs(void) {
    long runs = 0;
    for (int n = 1; n <= 6; n++)
        for (unsigned seed = 1; seed <= (unsigned)harness_iters(300); seed++) {
            long msgs = 0;
            if (!run(n, 5, seed, &msgs)) return;
            long want = 2L * (n - 1) * n * 5;
            if (msgs != want) { CHECK(0, "n=%d seed=%u: %ld messages for %d entries, expected 2(N-1) per entry = %ld", n, seed, msgs, n * 5, want); return; }
            runs++;
        }
    printf("    no violation observed in %ld random runs (this is evidence, not a proof; README section 4 has the proof)\n", runs);
}

int main(void) {
    harness_watchdog(50);
    harness_hint("request: WANTED, clock++, my_ts = clock, REQUEST to all others. On REQUEST: defer or reply. On REPLY: count; n-1 -> HELD. Release: reply to all deferred.");
    RUN_TEST(test_many_runs);
    return harness_report();
}
