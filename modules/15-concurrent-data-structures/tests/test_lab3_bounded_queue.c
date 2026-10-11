// TIMEOUT: 90
#include "atomics.h"
#include "harness.h"

typedef struct node {
    int value;
    struct node *next;
} node_t;
typedef struct {
    node_t *head;
    node_t *tail;
    pthread_mutex_t enqLock;
    pthread_mutex_t deqLock;
    pthread_cond_t notFull;
    pthread_cond_t notEmpty;
    shared_int permits;
    int capacity;
} bounded_queue_t;

void queue_init(bounded_queue_t *q, int cap);
void queue_destroy(bounded_queue_t *q);
void queue_enq(bounded_queue_t *q, int value);
int queue_deq(bounded_queue_t *q);

static void test_fifo_single_thread(void) {
    bounded_queue_t q;
    queue_init(&q, 5);
    for (int i = 1; i <= 5; i++) queue_enq(&q, i);
    CHECK(LOAD(q.permits) == 0, "after 5 enqueues into a queue of capacity 5, permits = %d, "
                                "expected 0", LOAD(q.permits));
    for (int i = 1; i <= 5; i++) {
        int v = queue_deq(&q);
        CHECK(v == i, "dequeue number %d returned %d, expected %d (first in, first out)", i, v, i);
    }
    CHECK(LOAD(q.permits) == 5, "after emptying the queue, permits = %d, expected 5",
          LOAD(q.permits));
    for (int r = 0; r < 20; r++) {   // mixed: keeps the sentinel moving
        queue_enq(&q, 100 + r);
        queue_enq(&q, 200 + r);
        int a = queue_deq(&q), b = queue_deq(&q);
        if (a != 100 + r || b != 200 + r) {
            CHECK(0, "enq %d, enq %d, then two deq gave %d and %d", 100 + r, 200 + r, a, b);
            break;
        }
    }
    queue_destroy(&q);
}

// ---- blocking ------------------------------------------------------------------------
static bounded_queue_t bq;
static atomic_int returned;
static int deq_result;

static void *enq_three(void *v) {
    (void)v;
    queue_enq(&bq, 3);
    atomic_store(&returned, 1);
    return NULL;
}
static void *deq_one(void *v) {
    (void)v;
    deq_result = queue_deq(&bq);
    atomic_store(&returned, 1);
    return NULL;
}

static void test_enq_waits_when_full(void) {
    queue_init(&bq, 2);
    queue_enq(&bq, 1);
    queue_enq(&bq, 2);
    atomic_store(&returned, 0);
    pthread_t t;
    pthread_create(&t, NULL, enq_three, NULL);
    usleep(150000);
    CHECK(!atomic_load(&returned), "enq into a FULL queue returned without waiting (capacity "
                                   "exceeded)");
    CHECK_EQ(queue_deq(&bq), 1);   // makes room: the waiting enq must now finish
    pthread_join(t, NULL);
    CHECK_EQ(queue_deq(&bq), 2);
    CHECK_EQ(queue_deq(&bq), 3);
    queue_destroy(&bq);
}

static void test_deq_waits_when_empty(void) {
    queue_init(&bq, 4);
    atomic_store(&returned, 0);
    pthread_t t;
    pthread_create(&t, NULL, deq_one, NULL);
    usleep(150000);
    CHECK(!atomic_load(&returned), "deq on an EMPTY queue returned without waiting");
    queue_enq(&bq, 7);             // the waiting deq must wake up and get 7
    pthread_join(t, NULL);
    CHECK_EQ(deq_result, 7);
    queue_destroy(&bq);
}

// ---- many producers and consumers -----------------------------------------------------------
#define MAXP 8
#define PER 6000
static atomic_int seen[MAXP * PER];
static atomic_int order_errors, bad_value, stop_monitor;
static atomic_int min_permits, max_permits;
static int nprod;

static void producer(int id, void *ctx) {
    (void)ctx;
    for (int i = 0; i < PER; i++) {
        queue_enq(&bq, id * PER + i);
        if ((i & 127) == 0) harness_jitter();
    }
}

static void *consumer(void *v) {
    (void)v;
    int last[MAXP];
    for (int p = 0; p < MAXP; p++) last[p] = -1;
    for (;;) {
        int x = queue_deq(&bq);
        if (x == -1) return NULL;   // poison pill
        if (x < 0 || x >= nprod * PER) { atomic_store(&bad_value, 1); continue; }
        atomic_fetch_add(&seen[x], 1);
        int p = x / PER, i = x % PER;
        if (i <= last[p]) atomic_fetch_add(&order_errors, 1);   // FIFO per producer
        last[p] = i;
    }
}

static void *monitor(void *v) {
    (void)v;
    while (!atomic_load(&stop_monitor)) {
        int p = LOAD(bq.permits);
        if (p < atomic_load(&min_permits)) atomic_store(&min_permits, p);
        if (p > atomic_load(&max_permits)) atomic_store(&max_permits, p);
    }
    return NULL;
}

static void many(int np, int nc, int cap) {
    queue_init(&bq, cap);
    nprod = np;
    for (int i = 0; i < MAXP * PER; i++) atomic_store(&seen[i], 0);
    atomic_store(&order_errors, 0);
    atomic_store(&bad_value, 0);
    atomic_store(&stop_monitor, 0);
    atomic_store(&min_permits, cap);
    atomic_store(&max_permits, 0);
    pthread_t ct[MAXP], mt;
    pthread_create(&mt, NULL, monitor, NULL);
    for (int i = 0; i < nc; i++) pthread_create(&ct[i], NULL, consumer, NULL);
    harness_parallel(np, producer, NULL);
    for (int i = 0; i < nc; i++) queue_enq(&bq, -1);
    for (int i = 0; i < nc; i++) pthread_join(ct[i], NULL);
    atomic_store(&stop_monitor, 1);
    pthread_join(mt, NULL);
    char what[64];
    snprintf(what, sizeof what, "%d producers, %d consumers, capacity %d", np, nc, cap);
    CHECK(!atomic_load(&bad_value), "%s: a consumer got a value nobody enqueued", what);
    for (int i = 0; i < np * PER; i++)
        if (atomic_load(&seen[i]) != 1) {
            CHECK(0, "%s: item %d was dequeued %d times (must be exactly once)", what, i,
                  atomic_load(&seen[i]));
            break;
        }
    CHECK(atomic_load(&order_errors) == 0, "%s: %d times a consumer got items of one producer "
                                           "out of order (not first-in first-out)", what,
          atomic_load(&order_errors));
    CHECK(atomic_load(&min_permits) >= 0 && atomic_load(&max_permits) <= cap,
          "%s: permits went outside 0..%d (seen %d..%d): the capacity was exceeded or an item "
          "was taken twice", what, cap, atomic_load(&min_permits), atomic_load(&max_permits));
    queue_destroy(&bq);
}

static void test_many_producers_consumers(void) {
    many(1, 1, 10);
    many(2, 2, 1);
    many(4, 4, 3);
    many(4, 2, 50);
    many(2, 6, 4);
}

// Consumers fall asleep on an empty queue BEFORE the producers start. With
// "wake only when the queue was empty" + pthread_cond_signal, some consumer
// can sleep forever (a lost wake-up): the watchdog then reports a timeout.
static void test_lost_wakeup(void) {
    int rounds = (int)harness_iters(25);
    for (int r = 0; r < rounds; r++) {
        queue_init(&bq, 2);
        nprod = 6;
        for (int i = 0; i < 6 * PER; i++) atomic_store(&seen[i], 0);
        pthread_t ct[6];
        for (int i = 0; i < 6; i++) pthread_create(&ct[i], NULL, consumer, NULL);
        usleep(2000);   // let the consumers block on the empty queue
        for (int i = 0; i < 6; i++) queue_enq(&bq, i * PER);   // 6 items, 6 sleepers
        for (int i = 0; i < 6; i++) queue_enq(&bq, -1);
        for (int i = 0; i < 6; i++) pthread_join(ct[i], NULL);
        for (int i = 0; i < 6; i++)
            if (atomic_load(&seen[i * PER]) != 1) {
                CHECK(0, "round %d: item %d dequeued %d times", r, i * PER,
                      atomic_load(&seen[i * PER]));
                r = rounds;
                break;
            }
        queue_destroy(&bq);
    }
    printf("    (%d rounds with sleeping consumers: nobody slept forever)\n", rounds);
}

int main(void) {
    harness_watchdog(80);
    harness_hint("Wait in a while loop, not an if. Wake sleepers with pthread_cond_broadcast, "
                 "and hold the OTHER side's lock while you broadcast. A timeout means a thread "
                 "slept forever (lost wake-up) or a lock was never released.");
    RUN_TEST(test_fifo_single_thread);
    if (h_checks_failed) {   // no point running threads on a queue that is wrong alone
        printf("  (skipping the threaded tests until the single-thread test passes)\n");
        return harness_report();
    }
    RUN_TEST(test_enq_waits_when_full);
    RUN_TEST(test_deq_waits_when_empty);
    RUN_TEST(test_many_producers_consumers);
    RUN_TEST(test_lost_wakeup);
    return harness_report();
}
