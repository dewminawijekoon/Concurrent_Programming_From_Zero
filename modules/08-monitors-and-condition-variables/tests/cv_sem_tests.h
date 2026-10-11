// Shared tests for a counting semaphore with cv_sem_init/down/up (ex2 and ex3).
#include "harness.h"
#include "cv_helpers.h"
#include "ex2_cv_semaphore.h"

// Each test uses its own semaphore, so a thread left stuck by one failing
// test cannot disturb the next one.
static cv_sem s1, s2, s3;
static void job_down(void *arg) { cv_sem_down((cv_sem *)arg); }

// Start value 2: two downs pass straight away, the third must wait for an up.
static void test_counts_permits(void) {
    cv_sem_init(&s1, 2);
    cvt_job *a = cvt_start(job_down, &s1);
    cvt_job *b = cvt_start(job_down, &s1);
    CHECK(cvt_wait(a, 5.0) && cvt_wait(b, 5.0),
          "with value 2, the first two downs should not wait (did you initialise value?)");
    cvt_job *c = cvt_start(job_down, &s1);
    sleep_ms(150);
    CHECK(!cvt_done(c), "a third down() went through although the value was 0: down() must wait "
                        "while there are no permits");
    cv_sem_up(&s1);
    CHECK(cvt_wait(c, 5.0), "the waiting down() did not wake up after up() (does up() signal?)");
}

// Start value 0: down() waits until another thread calls up().
static void test_zero_blocks(void) {
    cv_sem_init(&s2, 0);
    cvt_job *j = cvt_start(job_down, &s2);
    sleep_ms(150);
    CHECK(!cvt_done(j), "down() on a semaphore with value 0 returned without waiting");
    cv_sem_up(&s2);
    CHECK(cvt_wait(j, 5.0), "down() did not wake up after up()");
}

// up() before down(): the permit must be remembered (no lost wake-up).
static void test_up_first_is_remembered(void) {
    cv_sem_init(&s3, 0);
    cv_sem_up(&s3);
    cvt_job *j = cvt_start(job_down, &s3);
    CHECK(cvt_wait(j, 5.0), "up() then down(): the down() waited forever. The permit from up() "
                            "must be stored in value, not only signalled.");
}

// Multiplex (L09 slide 27): start value 3, 8 threads; never more than 3 inside.
static cv_sem room;
static atomic_int inside, most_inside;
static long rounds;
static void visitor(int id, void *ctx) {
    (void)id; (void)ctx;
    for (long k = 0; k < rounds; k++) {
        cv_sem_down(&room);
        int now = atomic_fetch_add(&inside, 1) + 1;
        int old = atomic_load(&most_inside);
        while (now > old && !atomic_compare_exchange_weak(&most_inside, &old, now)) {
        }
        harness_jitter();
        atomic_fetch_sub(&inside, 1);
        cv_sem_up(&room);
    }
}
static void run_multiplex(void *arg) { (void)arg; harness_parallel(8, visitor, NULL); }

static void test_multiplex_limit(void) {
    cv_sem_init(&room, 3);
    atomic_store(&inside, 0);
    atomic_store(&most_inside, 0);
    rounds = harness_iters(20000);
    cvt_job *j = cvt_start(run_multiplex, NULL);
    if (!cvt_wait(j, 30.0)) {
        CHECK(0, "8 threads using a semaphore with value 3 got stuck: a permit was lost somewhere");
        return;
    }
    CHECK(atomic_load(&most_inside) <= 3, "up to %d threads were inside together; the limit is 3",
          atomic_load(&most_inside));
    CHECK_EQ(room.value, 3);
    if (h_checks_failed == 0)
        printf("  (8 threads x %ld entries: never more than %d inside; no violation observed - "
               "a stress test cannot prove correctness)\n", rounds, atomic_load(&most_inside));
}

static int run_cv_sem_tests(void) {
    harness_watchdog(50);
    harness_hint("down: lock; while (value == 0) pthread_cond_wait(&positive, &m); value--; unlock. "
                 "up: lock; value++; pthread_cond_signal(&positive); unlock. "
                 "init: set value and initialise the mutex and condition variable.");
    RUN_TEST(test_counts_permits);
    RUN_TEST(test_zero_blocks);
    RUN_TEST(test_up_first_is_remembered);
    RUN_TEST(test_multiplex_limit);
    return harness_report();
}
