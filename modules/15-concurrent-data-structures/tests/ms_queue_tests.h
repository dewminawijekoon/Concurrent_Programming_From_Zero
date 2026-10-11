// Shared tests for the Michael-Scott queue (ex2 and ex3).
#include "harness.h"
#include "ms_queue.h"

static void test_fifo_single_thread(void) {
    msq_t q;
    msq_init(&q, 100);
    long v = -1;
    CHECK(!msq_deq(&q, &v), "deq on an empty queue must return false");
    for (long i = 1; i <= 5; i++) msq_enq(&q, i * 10);
    for (long i = 1; i <= 5; i++) {
        v = -1;
        bool ok = msq_deq(&q, &v);
        CHECK(ok && v == i * 10, "dequeue number %ld: got ok=%d value=%ld, expected %ld", i, ok,
              v, i * 10);
    }
    CHECK(!msq_deq(&q, &v), "after taking all 5 items, deq must return false");
    msq_enq(&q, 77);
    CHECK(msq_deq(&q, &v) && v == 77, "enq after the queue became empty again");
    msq_destroy(&q);
}

// Run fn in a helper thread; give up after 2 seconds (a missing "help" step
// makes enq spin forever when the tail lags).
static int run_with_timeout(void *(*fn)(void *), void *arg) {
    pthread_t t;
    pthread_create(&t, NULL, fn, arg);
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += 2;
    if (pthread_timedjoin_np(t, NULL, &ts) != 0) return 0;
    return 1;
}

static msq_t lq;
static long got_value;
static void *enq42(void *v) { (void)v; msq_enq(&lq, 42); return NULL; }
static void *deq_any(void *v) { (void)v; got_value = -1; msq_deq(&lq, &got_value); return NULL; }

// Build "logical enqueue done, physical not yet": sentinel -> n1, but tail
// still points to the sentinel. This is what another thread sees between
// an enqueuer's two CAS steps.
static void make_lagging(void) {
    msq_init(&lq, 10);
    msq_node *n1 = msq_new_node(&lq, 41);
    STORE(LOAD(lq.tail)->next, n1);
}

static void test_enq_helps_lagging_tail(void) {
    make_lagging();
    if (!run_with_timeout(enq42, NULL)) {
        CHECK(0, "enq got stuck when the tail was lagging (tail->next was not NULL). It must "
                 "help: CAS the tail forward, then try again");
        harness_report();
        _exit(1);   // the stuck thread cannot be stopped; end the test here
    }
    long a = -1, b = -1;
    msq_deq(&lq, &a);
    msq_deq(&lq, &b);
    CHECK(a == 41 && b == 42, "after enq(42) on a lagging tail: dequeued %ld then %ld, expected "
                              "41 then 42", a, b);
    msq_destroy(&lq);
}

static void test_deq_helps_lagging_tail(void) {
    make_lagging();
    if (!run_with_timeout(deq_any, NULL)) {
        CHECK(0, "deq got stuck when the tail was lagging");
        harness_report();
        _exit(1);
    }
    CHECK(got_value == 41, "deq with a lagging tail returned %ld, expected 41", got_value);
    CHECK(LOAD(lq.tail) == LOAD(lq.head),
          "after that deq, head moved past tail: when head == tail but head->next != NULL, deq "
          "must first move the tail forward (help), then try again");
    msq_destroy(&lq);
}

// ---- concurrent producers and consumers ---------------------------------------------
#define P 4
#define C 4
#define PER 20000
static msq_t cq;
static atomic_int seen[P * PER];
static atomic_long taken;
static atomic_int order_errors, bad_value;

static void prod_cons(int id, void *ctx) {
    (void)ctx;
    if (id < P) {
        for (int i = 0; i < PER; i++) {
            msq_enq(&cq, (long)id * PER + i);
            if ((i & 255) == 0) harness_jitter();
        }
        return;
    }
    long last[P];
    for (int p = 0; p < P; p++) last[p] = -1;
    while (atomic_load(&taken) < P * PER) {
        long v;
        if (!msq_deq(&cq, &v)) { sched_yield(); continue; }
        atomic_fetch_add(&taken, 1);
        if (v < 0 || v >= P * PER) { atomic_store(&bad_value, 1); continue; }
        atomic_fetch_add(&seen[v], 1);
        int p = (int)(v / PER);
        if (v % PER <= last[p]) atomic_fetch_add(&order_errors, 1);
        last[p] = v % PER;
    }
}

static void test_concurrent_exactly_once(void) {
    msq_init(&cq, P * PER + 1);
    harness_parallel(P + C, prod_cons, NULL);
    CHECK(!atomic_load(&bad_value), "a consumer got a value nobody enqueued");
    for (int i = 0; i < P * PER; i++)
        if (atomic_load(&seen[i]) != 1) {
            CHECK(0, "item %d was dequeued %d times (must be exactly once)", i,
                  atomic_load(&seen[i]));
            break;
        }
    CHECK(atomic_load(&order_errors) == 0, "%d times a consumer saw one producer's items out of "
                                           "order", atomic_load(&order_errors));
    long v;
    CHECK(!msq_deq(&cq, &v), "the queue must be empty at the end");
    msq_destroy(&cq);
}

// Each thread does enq then deq, again and again. A deq can then never find
// the queue empty, because every thread enqueued before it dequeues.
#define PAIRS 20000
static atomic_int empty_seen;
static atomic_long sum_in, sum_out;
static void pairs(int id, void *ctx) {
    (void)ctx;
    for (int i = 0; i < PAIRS; i++) {
        long x = (long)id * PAIRS + i;
        msq_enq(&cq, x);
        atomic_fetch_add(&sum_in, x);
        long v;
        if (msq_deq(&cq, &v)) atomic_fetch_add(&sum_out, v);
        else atomic_fetch_add(&empty_seen, 1);
    }
}

static void test_enq_deq_pairs(void) {
    msq_init(&cq, 4L * PAIRS + 1);
    harness_parallel(4, pairs, NULL);
    CHECK(atomic_load(&empty_seen) == 0, "deq said 'empty' %d times, but every thread enqueued "
                                         "before dequeuing", atomic_load(&empty_seen));
    CHECK(atomic_load(&sum_in) == atomic_load(&sum_out), "sum of dequeued values differs from "
                                                         "the sum of enqueued values");
    msq_destroy(&cq);
}

static int run_ms_queue_tests(void) {
    harness_watchdog(50);
    harness_hint("Re-read the value of tail and next in every loop round. If tail->next is not "
                 "NULL the tail lags: CAS it forward and retry. In deq, read next->value before "
                 "the CAS on head.");
    RUN_TEST(test_fifo_single_thread);
    if (h_checks_failed) {
        printf("  (skipping the threaded tests until the single-thread test passes)\n");
        return harness_report();
    }
    RUN_TEST(test_enq_helps_lagging_tail);
    RUN_TEST(test_deq_helps_lagging_tail);
    RUN_TEST(test_concurrent_exactly_once);
    RUN_TEST(test_enq_deq_pairs);
    if (h_checks_failed == 0)
        printf("  (no lost, doubled or reordered item observed - a stress test cannot prove a "
               "lock-free algorithm correct)\n");
    return harness_report();
}
