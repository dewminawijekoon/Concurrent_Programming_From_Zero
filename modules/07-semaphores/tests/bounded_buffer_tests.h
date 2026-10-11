// Shared tests for exercises 4 and 5 (the bounded buffer).
#include "ex4_bounded_buffer.h"
#include "sem_test_util.h"

static void test_fifo_single_thread(void) {
    CHECK(bb_init(4) == 0, "bb_init(4) failed");
    for (int i = 1; i <= 4; i++) bb_put(10 * i);
    for (int i = 1; i <= 4; i++) {
        int got = bb_get();
        CHECK(got == 10 * i, "item %d: got %d, expected %d (first in, first out)", i, got, 10 * i);
    }
    // wrap around the end of the circular array a few times
    for (int round = 0; round < 5; round++) {
        bb_put(round);
        bb_put(round + 100);
        int a = bb_get(), b = bb_get();
        CHECK(a == round && b == round + 100,
              "after wrapping around: got %d, %d, expected %d, %d", a, b, round, round + 100);
    }
    bb_destroy();
}

static atomic_int done_flag;
static int got_value;
static void *getter(void *arg) {
    (void)arg;
    got_value = bb_get();
    atomic_store(&done_flag, 1);
    return NULL;
}
static void *putter(void *arg) {
    bb_put((int)(long)arg);
    atomic_store(&done_flag, 1);
    return NULL;
}

static void test_get_waits_when_empty(void) {
    bb_init(2);
    atomic_store(&done_flag, 0);
    pthread_t t;
    pthread_create(&t, NULL, getter, NULL);
    sleep_ms(100);
    CHECK(!atomic_load(&done_flag), "bb_get() returned on an EMPTY buffer instead of waiting");
    bb_put(7);
    CHECK(wait_for_flag(&done_flag, 2000), "an item was put, but the waiting bb_get() never returned");
    pthread_join(t, NULL);
    CHECK(got_value == 7, "the waiting bb_get() returned %d, expected 7", got_value);
    bb_destroy();
}

static void test_put_waits_when_full(void) {
    bb_init(2);
    bb_put(1);
    bb_put(2);
    atomic_store(&done_flag, 0);
    pthread_t t;
    pthread_create(&t, NULL, putter, (void *)3L);
    sleep_ms(100);
    CHECK(!atomic_load(&done_flag), "bb_put() returned on a FULL buffer instead of waiting");
    int a = bb_get();
    CHECK(wait_for_flag(&done_flag, 2000), "a slot was freed, but the waiting bb_put() never returned");
    pthread_join(t, NULL);
    int b = bb_get(), c = bb_get();
    CHECK(a == 1 && b == 2 && c == 3, "got %d %d %d, expected 1 2 3", a, b, c);
    bb_destroy();
}

// ---- stress tests ----
static long per_thread;
static atomic_long wrong;
static atomic_int *seen;

static void spsc(int id, void *ctx) {
    (void)ctx;
    for (long k = 0; k < per_thread; k++) {
        if (id == 0) {
            if ((k & 255) == 0) harness_jitter();
            bb_put((int)k);
        } else {
            int got = bb_get();
            if (got != (int)k && atomic_fetch_add(&wrong, 1) == 0)
                printf("    consumer got %d, expected %ld\n", got, k);
        }
    }
}

static void test_one_producer_one_consumer(void) {
    bb_init(8);
    per_thread = harness_iters(200000);
    atomic_store(&wrong, 0);
    harness_parallel(2, spsc, NULL);
    bb_destroy();
    CHECK(atomic_load(&wrong) == 0, "%ld items came out wrong or out of order", atomic_load(&wrong));
    if (atomic_load(&wrong) == 0)
        printf("  (%ld items passed through in order - no problem observed)\n", per_thread);
}

#define PRODUCERS 4
#define CONSUMERS 4
static void mpmc(int id, void *ctx) {
    (void)ctx;
    for (long k = 0; k < per_thread; k++) {
        if (id < PRODUCERS) {
            harness_jitter();
            bb_put((int)(id * per_thread + k));   // every item is a different number
        } else {
            int got = bb_get();
            if (got < 0 || got >= PRODUCERS * per_thread) {
                if (atomic_fetch_add(&wrong, 1) == 0) printf("    consumer got %d, which was never put\n", got);
            } else {
                atomic_fetch_add(&seen[got], 1);
            }
        }
    }
}

static void test_many_producers_many_consumers(void) {
    bb_init(8);
    per_thread = harness_iters(30000);
    long total = PRODUCERS * per_thread;
    seen = calloc((size_t)total, sizeof *seen);
    atomic_store(&wrong, 0);
    harness_parallel(PRODUCERS + CONSUMERS, mpmc, NULL);
    bb_destroy();
    long lost = 0, twice = 0;
    for (long i = 0; i < total; i++) {
        int s = atomic_load(&seen[i]);
        if (s == 0) lost++;
        if (s > 1) twice++;
    }
    free(seen);
    CHECK(atomic_load(&wrong) == 0, "consumers got %ld values that no producer ever put",
          atomic_load(&wrong));
    CHECK(lost == 0 && twice == 0,
          "with %d producers and %d consumers: %ld items lost, %ld taken more than once - "
          "two threads used the same slot at the same time (is buf/head/tail protected?)",
          PRODUCERS, CONSUMERS, lost, twice);
    if (lost == 0 && twice == 0 && atomic_load(&wrong) == 0)
        printf("  (every one of %ld items was taken exactly once - no problem observed)\n", total);
}

static int run_bounded_buffer_tests(void) {
    harness_watchdog(45);
    harness_hint("spaces starts at the capacity, elements at 0, mutex at 1. "
                 "put: wait(spaces); wait(mutex); store; post(mutex); post(elements). "
                 "get is the mirror image. Wait on spaces/elements BEFORE taking the mutex.");
    RUN_TEST(test_fifo_single_thread);
    RUN_TEST(test_get_waits_when_empty);
    RUN_TEST(test_put_waits_when_full);
    RUN_TEST(test_one_producer_one_consumer);
    RUN_TEST(test_many_producers_many_consumers);
    return harness_report();
}
