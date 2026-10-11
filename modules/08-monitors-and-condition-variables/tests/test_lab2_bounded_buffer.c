// TIMEOUT: 60
// Checks lab2_bounded_buffer: blocking when full/empty, order, no overflow.
#include "harness.h"
#include "cv_helpers.h"

void bb_init(int buffer_size);
void put(int item);
int get(void);
extern int count, size;
extern pthread_mutex_t mutex;

static int got_value;
static void put_99(void *arg) { (void)arg; put(99); }
static void get_one(void *arg) { (void)arg; got_value = get(); }

// Fill the buffer; one more put() must wait until get() makes room.
static void test_put_waits_when_full(void) {
    bb_init(3);
    for (int i = 0; i < 3; i++) put(i);          // room for 3: these must not block
    cvt_job *j = cvt_start(put_99, NULL);
    sleep_ms(150);
    CHECK(!cvt_done(j), "put() returned although the buffer was full: the producer must wait");
    int a = get();
    CHECK_EQ(a, 0);
    if (!cvt_wait(j, 5.0)) {
        CHECK(0, "the waiting put() did not finish after get() made room (did get() signal not_full?)");
        return;
    }
    int b = get(), c = get(), d = get();
    CHECK_EQ(b, 1);
    CHECK_EQ(c, 2);
    CHECK_EQ(d, 99);
}

// get() on an empty buffer must wait until put() adds something.
static void test_get_waits_when_empty(void) {
    bb_init(2);
    got_value = -12345;
    cvt_job *j = cvt_start(get_one, NULL);
    sleep_ms(150);
    CHECK(!cvt_done(j), "get() returned although the buffer was empty: the consumer must wait");
    put(7);
    CHECK(cvt_wait(j, 5.0), "the waiting get() did not finish after put() "
                            "(did put() signal not_empty?)");
    CHECK_EQ(got_value, 7);
}

// ---- one producer, one consumer, many items ----
static int n_items;
static atomic_int stop_sampler;
static atomic_long bad_samples, samples;

static void producer(int id, void *ctx) {
    (void)id; (void)ctx;
    for (int i = 0; i < n_items; i++) {
        put(i);
        if ((i & 255) == 0) harness_jitter();
    }
    put(-1);
}

static long received, first_wrong = -1, wrong_value;
static void consumer(void) {
    received = 0;
    first_wrong = -1;
    for (;;) {
        int item = get();
        if (item == -1) break;
        if (item != received && first_wrong < 0) { first_wrong = received; wrong_value = item; }
        received++;
        if ((received & 255) == 0) harness_jitter();
    }
}

// A third thread looks at `count` (holding your mutex) again and again:
// it must always be between 0 and the buffer size.
static void *sampler(void *arg) {
    (void)arg;
    while (!atomic_load(&stop_sampler)) {
        pthread_mutex_lock(&mutex);
        if (count < 0 || count > size) atomic_fetch_add(&bad_samples, 1);
        pthread_mutex_unlock(&mutex);
        atomic_fetch_add(&samples, 1);
        sched_yield();
    }
    return NULL;
}

static void *producer_thread(void *arg) { producer(0, arg); return NULL; }

static void run_spsc(int bsize) {
    bb_init(bsize);
    n_items = (int)harness_iters(100000);
    atomic_store(&stop_sampler, 0);
    atomic_store(&bad_samples, 0);
    pthread_t p, s;
    pthread_create(&s, NULL, sampler, NULL);
    pthread_create(&p, NULL, producer_thread, NULL);
    consumer();
    pthread_join(p, NULL);
    atomic_store(&stop_sampler, 1);
    pthread_join(s, NULL);
    CHECK(received == n_items, "buffer size %d: the consumer got %ld items before the -1, expected %d",
          bsize, received, n_items);
    CHECK(first_wrong < 0, "buffer size %d: item number %ld should have been %ld but was %ld "
          "(an item was overwritten, lost or read twice)", bsize, first_wrong, first_wrong, wrong_value);
    CHECK(atomic_load(&bad_samples) == 0, "buffer size %d: count went outside 0..%d in %ld samples",
          bsize, bsize, atomic_load(&bad_samples));
}

static void test_all_items_in_order(void) {
    int sizes[] = {1, 2, 5, 64};
    for (int i = 0; i < 4; i++) run_spsc(sizes[i]);
    if (h_checks_failed == 0)
        printf("  (all %d items arrived in order for buffer sizes 1, 2, 5, 64; count stayed in range "
               "in %ld samples - a stress test cannot prove correctness)\n", n_items, atomic_load(&samples));
}

int main(void) {
    harness_watchdog(50);
    harness_hint("put() must wait WHILE count == size, get() WHILE count == 0, both holding the mutex. "
                 "After changing count, signal the other condition variable.");
    RUN_TEST(test_get_waits_when_empty);
    RUN_TEST(test_put_waits_when_full);
    RUN_TEST(test_all_items_in_order);
    return harness_report();
}
