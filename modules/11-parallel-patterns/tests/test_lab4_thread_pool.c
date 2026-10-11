// BUILD: -lm
// TIMEOUT: 60
#include "harness.h"

typedef struct {
    int id;
    int difficulty;
    double result;
} task_t;
typedef struct pool pool_t;

extern atomic_int *task_done_count;
void do_task(task_t *t);
void tasks_generate(task_t *t, int n, unsigned seed, int easy, int hard);
void run_serial(task_t *t, int n);
void run_counter_pool(task_t *t, int n, int p);
pool_t *pool_create(int nthreads);
void pool_submit(pool_t *p, void (*fn)(void *), void *arg);
void pool_wait(pool_t *p);
void pool_destroy(pool_t *p);

#define NT 300
static task_t ref[NT], t[NT];
static atomic_int done[NT + 64];

static void reset_counts(void) {
    for (int i = 0; i < NT + 64; i++) atomic_store(&done[i], 0);
}

static int check_all(const char *what, int n) {
    for (int i = 0; i < n; i++) {
        int c = atomic_load(&done[i]);
        if (c != 1) {
            CHECK(0, "%s: task %d was done %d times (must be exactly once)", what, i, c);
            return 0;
        }
        if (t[i].result != ref[i].result) {
            CHECK(0, "%s: task %d has a wrong result", what, i);
            return 0;
        }
    }
    CHECK(1, "unreachable");
    return 1;
}

static void prepare(unsigned seed) {
    tasks_generate(ref, NT, seed, 50, 5000);
    task_done_count = NULL;
    run_serial(ref, NT);
    tasks_generate(t, NT, seed, 50, 5000);
    task_done_count = done;
    reset_counts();
}

static void test_counter_pool(void) {
    int ps[] = {1, 2, 3, 4, 8};
    for (int k = 0; k < 5; k++) {
        prepare(10u + (unsigned)k);
        run_counter_pool(t, NT, ps[k]);
        char what[64];
        snprintf(what, sizeof what, "Part A with %d threads", ps[k]);
        if (!check_all(what, NT)) return;
    }
}

static void task_fn(void *arg) { do_task(arg); }

static void test_pool_basic(void) {
    int ps[] = {1, 2, 4, 8};
    for (int k = 0; k < 4; k++) {
        prepare(20u + (unsigned)k);
        pool_t *p = pool_create(ps[k]);
        if (!p) {
            CHECK(0, "pool_create returned NULL");
            return;
        }
        for (int i = 0; i < NT; i++) pool_submit(p, task_fn, &t[i]);
        pool_wait(p);
        char what[64];
        snprintf(what, sizeof what, "Part B with %d threads, after pool_wait", ps[k]);
        int ok = check_all(what, NT);
        pool_destroy(p);
        if (!ok) return;
    }
}

// The pool must be reusable after pool_wait, and pool_wait with nothing to do returns at once.
static void test_pool_reuse(void) {
    prepare(31);
    pool_t *p = pool_create(3);
    if (!p) {
        CHECK(0, "pool_create returned NULL");
        return;
    }
    pool_wait(p);
    for (int i = 0; i < NT / 2; i++) pool_submit(p, task_fn, &t[i]);
    pool_wait(p);
    for (int i = NT / 2; i < NT; i++) pool_submit(p, task_fn, &t[i]);
    pool_wait(p);
    check_all("Part B, two batches with pool_wait in between", NT);
    pool_destroy(p);
}

// Tasks that submit more tasks (divide & conquer inside a pool).
typedef struct {
    pool_t *p;
    int first, count;
} split_arg;
static split_arg splits[NT];
static atomic_int nsplits;

static void split_fn(void *v) {
    split_arg *a = v;
    if (a->count <= 8) {
        for (int i = a->first; i < a->first + a->count; i++) do_task(&t[i]);
        return;
    }
    int half = a->count / 2;
    for (int h = 0; h < 2; h++) {
        int k = atomic_fetch_add(&nsplits, 1);
        splits[k] = (split_arg){a->p, a->first + h * half, h ? a->count - half : half};
        pool_submit(a->p, split_fn, &splits[k]);
    }
}

static void test_pool_nested(void) {
    prepare(41);
    pool_t *p = pool_create(4);
    if (!p) {
        CHECK(0, "pool_create returned NULL");
        return;
    }
    atomic_store(&nsplits, 1);
    splits[0] = (split_arg){p, 0, NT};
    pool_submit(p, split_fn, &splits[0]);
    pool_wait(p);   // must not return before the tasks submitted by tasks are done
    check_all("Part B, tasks that submit more tasks", NT);
    pool_destroy(p);
}

// Shutting down must wake workers that are asleep waiting for work.
static void test_pool_shutdown(void) {
    int rounds = (int)harness_iters(40);
    for (int r = 0; r < rounds; r++) {
        prepare(50u + (unsigned)r);
        pool_t *p = pool_create(1 + r % 6);
        if (!p) {
            CHECK(0, "pool_create returned NULL");
            return;
        }
        int n = r % 4 == 0 ? 0 : 1 + r % 9;   // sometimes no tasks at all
        for (int i = 0; i < n; i++) pool_submit(p, task_fn, &t[i]);
        pool_destroy(p);                      // also waits for the tasks
        for (int i = 0; i < n; i++)
            if (atomic_load(&done[i]) != 1) {
                CHECK(0, "round %d: pool_destroy returned before task %d was done", r, i);
                return;
            }
    }
    CHECK(1, "unreachable");
    printf("    (%d create/destroy rounds finished cleanly)\n", rounds);
}

int main(void) {
    harness_watchdog(50);
    harness_hint("Run each task with the lock RELEASED. pending++ in submit, pending-- after "
                 "the task ran. A timeout in the shutdown test usually means pool_destroy used "
                 "signal instead of broadcast, or workers do not re-check `shutdown`.");
    RUN_TEST(test_counter_pool);
    RUN_TEST(test_pool_basic);
    RUN_TEST(test_pool_reuse);
    RUN_TEST(test_pool_nested);
    RUN_TEST(test_pool_shutdown);
    return harness_report();
}
