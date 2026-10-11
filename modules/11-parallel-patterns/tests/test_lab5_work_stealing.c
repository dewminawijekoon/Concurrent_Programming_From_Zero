// BUILD: -lm
// TIMEOUT: 60
#include "harness.h"

typedef struct {
    int id;
    int difficulty;
    double result;
} task_t;
typedef struct {
    int *items;
    int cap;
    int front, back;
    pthread_mutex_t lock;
} deque_t;

extern atomic_int *task_done_count;
void tasks_generate(task_t *t, int n, unsigned seed, int easy, int hard);
void run_serial(task_t *t, int n);
void deque_init(deque_t *d, int cap);
void deque_destroy(deque_t *d);
void deque_push(deque_t *d, int v);
int deque_pop(deque_t *d);
int deque_steal(deque_t *d);
void ws_run(task_t *tasks, int n, int p, long *steals);

static void test_deque_order(void) {
    deque_t d;
    deque_init(&d, 10);
    CHECK_EQ(deque_pop(&d), -1);
    CHECK_EQ(deque_steal(&d), -1);
    deque_push(&d, 1);
    deque_push(&d, 2);
    deque_push(&d, 3);
    deque_push(&d, 4);
    CHECK(deque_pop(&d) == 4, "pop must take the NEWEST item (the back)");
    CHECK(deque_steal(&d) == 1, "steal must take the OLDEST item (the front)");
    CHECK_EQ(deque_pop(&d), 3);
    CHECK_EQ(deque_steal(&d), 2);
    CHECK(deque_pop(&d) == -1 && deque_steal(&d) == -1, "an empty deque must give -1");
    deque_destroy(&d);
}

// The owner pushes and pops while thieves steal at the same time:
// every item must come out exactly once.
#define DN 40000
static deque_t sd;
static atomic_int taken[DN];
static atomic_int owner_done;
static atomic_long stolen;

static void deque_stress_worker(int id, void *ctx) {
    (void)ctx;
    if (id == 0) {   // the owner
        for (int i = 0; i < DN; i++) {
            deque_push(&sd, i);
            if (i % 3 == 2) {
                int v = deque_pop(&sd);
                if (v >= 0) atomic_fetch_add(&taken[v], 1);
            }
        }
        int v;
        while ((v = deque_pop(&sd)) >= 0) atomic_fetch_add(&taken[v], 1);
        atomic_store(&owner_done, 1);
    } else {         // thieves
        for (;;) {
            int done = atomic_load(&owner_done);
            int v = deque_steal(&sd);
            if (v >= 0) {
                atomic_fetch_add(&taken[v], 1);
                atomic_fetch_add(&stolen, 1);
            } else if (done) {
                break;
            } else {
                sched_yield();
            }
        }
    }
}

static void test_deque_concurrent(void) {
    deque_init(&sd, DN);
    for (int i = 0; i < DN; i++) atomic_store(&taken[i], 0);
    harness_parallel(4, deque_stress_worker, NULL);
    for (int i = 0; i < DN; i++) {
        int c = atomic_load(&taken[i]);
        if (c != 1) {
            CHECK(0, "item %d came out of the deque %d times (owner pops + thieves steal at the "
                     "same time; it must be exactly once)", i, c);
            break;
        }
    }
    printf("    (%ld of %d items were stolen by the 3 thieves)\n", atomic_load(&stolen), DN);
    deque_destroy(&sd);
}

#define NT 400
static task_t ref[NT], t[NT];
static atomic_int done[NT];

static int run_ws(int n, int p, unsigned seed) {
    tasks_generate(ref, n, seed, 20, 4000);
    task_done_count = NULL;
    run_serial(ref, n);
    tasks_generate(t, n, seed, 20, 4000);
    for (int i = 0; i < NT; i++) atomic_store(&done[i], 0);
    task_done_count = done;
    long steals[16];
    for (int w = 0; w < 16; w++) steals[w] = -1;
    ws_run(t, n, p, steals);
    for (int i = 0; i < n; i++) {
        int c = atomic_load(&done[i]);
        if (c != 1) {
            CHECK(0, "%d tasks, %d threads: task %d was done %d times (must be exactly once)", n,
                  p, i, c);
            return 0;
        }
        if (t[i].result != ref[i].result) {
            CHECK(0, "%d tasks, %d threads: task %d has a wrong result", n, p, i);
            return 0;
        }
    }
    for (int w = 0; w < p; w++)
        if (steals[w] < 0) {
            CHECK(0, "%d threads: steals[%d] was not filled in", p, w);
            return 0;
        }
    if (p == 1 && steals[0] != 0) {
        CHECK(0, "with 1 thread nobody can steal, but steals[0] = %ld", steals[0]);
        return 0;
    }
    CHECK(1, "unreachable");
    return 1;
}

static void test_ws_run(void) {
    int ps[] = {1, 2, 3, 4, 8};
    for (int k = 0; k < 5; k++)
        if (!run_ws(NT, ps[k], 3u + (unsigned)k)) return;
    // fewer tasks than threads, and no tasks at all
    run_ws(3, 8, 77);
    run_ws(0, 4, 78);
}

static void test_ws_stress(void) {
    int rounds = (int)harness_iters(30);
    for (int r = 0; r < rounds; r++)
        if (!run_ws(1 + (r * 37) % NT, 1 + r % 8, 100u + (unsigned)r)) return;
    printf("    (%d rounds: no lost or doubled task observed)\n", rounds);
}

int main(void) {
    harness_watchdog(50);
    harness_hint("pop: --back under the lock; steal: front++ under the lock; both return -1 "
                 "when back == front. A worker stops only after a steal attempt on every other "
                 "deque failed.");
    RUN_TEST(test_deque_order);
    RUN_TEST(test_deque_concurrent);
    RUN_TEST(test_ws_run);
    RUN_TEST(test_ws_stress);
    return harness_report();
}
