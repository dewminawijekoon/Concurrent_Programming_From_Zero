// BUILD: -lm
// TIMEOUT: 60
#include "harness.h"

typedef struct {
    int seq;
    long value;
} item_t;

extern atomic_int *stage2_calls;
extern pthread_t main_thread;
extern atomic_int stage2_on_main;
void pipeline_serial(int n, int work, item_t *out);
int pipeline_run(int n, int n_stage2, int qcap, int work, item_t *out);

#define N 600

static item_t ref[N], out[N];
static atomic_int calls[N];

static int run_one(int n, int k, int cap, int work) {
    memset(out, 0xff, sizeof out);
    for (int i = 0; i < N; i++) atomic_store(&calls[i], 0);
    pipeline_serial(n, work, ref);
    for (int i = 0; i < N; i++) atomic_store(&calls[i], 0);   // the serial run counted too,
    atomic_store(&stage2_on_main, 0);                         // and it ran in the main thread
    int got = pipeline_run(n, k, cap, work, out);
    char what[80];
    snprintf(what, sizeof what, "%d items, %d stage-2 threads, queue capacity %d", n, k, cap);
    if (got != n) {
        CHECK(0, "%s: %d items were emitted, expected %d", what, got, n);
        return 0;
    }
    for (int i = 0; i < n; i++) {
        int c = atomic_load(&calls[i]);
        if (c != 1) {
            CHECK(0, "%s: stage 2 processed item %d %d times (must be exactly once)", what, i, c);
            return 0;
        }
    }
    for (int i = 0; i < n; i++)
        if (out[i].seq != i || out[i].value != ref[i].value) {
            CHECK(0, "%s: emitted item number %d is {seq %d, value %ld}, expected {seq %d, "
                     "value %ld} - out of order, or wrong value", what, i, out[i].seq,
                  out[i].value, i, ref[i].value);
            return 0;
        }
    if (atomic_load(&stage2_on_main)) {
        CHECK(0, "%s: stage 2 ran in the main thread - each stage needs its own thread(s)", what);
        return 0;
    }
    CHECK(1, "unreachable");   // count one passed check per run
    return 1;
}

static void test_one_stage2_thread(void) {
    run_one(N, 1, 4, 50);
    run_one(N, 1, 1, 10);   // capacity 1: every put waits for a take
}

static void test_replicated_stage2(void) {
    int ks[] = {2, 3, 5};
    int caps[] = {1, 2, 8};
    for (int a = 0; a < 3; a++)
        for (int b = 0; b < 3; b++)
            if (!run_one(N, ks[a], caps[b], (a * 37 + b * 11) % 200)) return;
}

static void test_no_items(void) {
    run_one(0, 3, 2, 10);   // only poison goes through; nothing is emitted, nobody hangs
    run_one(1, 4, 1, 10);
}

static void test_repeat(void) {
    int rounds = (int)harness_iters(15);
    for (int r = 0; r < rounds; r++)
        if (!run_one(200, 1 + r % 4, 1 + r % 3, r % 3 == 0 ? 0 : 30)) return;
    printf("    (%d extra rounds: no lost, doubled or misordered item observed)\n", rounds);
}

int main(void) {
    harness_watchdog(50);
    harness_hint("Send one POISON item per stage-2 thread; each stage-2 thread forwards its "
                 "poison and stops; stage 3 stops after counting n_stage2 poisons. Stage 3 must "
                 "hold back early items and emit in seq order. A timeout usually means too few "
                 "poison items.");
    main_thread = pthread_self();
    stage2_calls = calls;
    RUN_TEST(test_one_stage2_thread);
    RUN_TEST(test_replicated_stage2);
    RUN_TEST(test_no_items);
    RUN_TEST(test_repeat);
    return harness_report();
}
