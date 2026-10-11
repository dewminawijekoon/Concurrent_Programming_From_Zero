// TIMEOUT: 60
// Checks ex1_gate: waiters block until the gate opens, ALL of them wake up,
// and a thread that arrives after the gate opened does not wait at all.
#include "harness.h"
#include "cv_helpers.h"

void gate_init(void);
void gate_wait(void);
void gate_open(void);

static void job_wait(void *arg) { (void)arg; gate_wait(); }

static void test_waiters_block_until_open(void) {
    gate_init();
    cvt_job *j[4];
    for (int i = 0; i < 4; i++) j[i] = cvt_start(job_wait, NULL);
    sleep_ms(150);
    int through = 0;
    for (int i = 0; i < 4; i++) through += cvt_done(j[i]);
    CHECK(through == 0, "%d of 4 threads went through gate_wait() before the gate was opened", through);
    gate_open();
    int woke = 0;
    for (int i = 0; i < 4; i++) woke += cvt_wait(j[i], 5.0);
    CHECK(woke == 4, "only %d of 4 waiting threads woke up after gate_open(). "
                     "pthread_cond_signal() wakes ONE thread; to wake all, use broadcast.", woke);
}

// Open first, wait later: the wake-up must not be lost.
static void test_open_before_wait(void) {
    gate_init();
    gate_open();
    cvt_job *j = cvt_start(job_wait, NULL);
    CHECK(cvt_wait(j, 5.0), "gate_wait() did not return although the gate was already open. "
                            "A signal sent when nobody waits is lost: remember the state in is_open "
                            "and only wait WHILE it is false.");
}

// Closing again (gate_init) must make new waiters block again.
static void test_reusable_after_init(void) {
    gate_init();
    gate_open();
    gate_init();
    cvt_job *j = cvt_start(job_wait, NULL);
    sleep_ms(150);
    CHECK(!cvt_done(j), "after gate_init() the gate should be closed again");
    gate_open();
    CHECK(cvt_wait(j, 5.0), "the waiter did not wake after the second gate_open()");
}

int main(void) {
    harness_watchdog(40);
    harness_hint("gate_wait: lock; while (!is_open) pthread_cond_wait(...); unlock. "
                 "gate_open: lock; is_open = true; pthread_cond_broadcast(...); unlock.");
    RUN_TEST(test_waiters_block_until_open);
    RUN_TEST(test_open_before_wait);
    RUN_TEST(test_reusable_after_init);
    return harness_report();
}
