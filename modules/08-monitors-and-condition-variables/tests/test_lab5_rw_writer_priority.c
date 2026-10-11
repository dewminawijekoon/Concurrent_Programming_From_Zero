// TIMEOUT: 60
// Checks lab5_rw_writer_priority: the reader-writer rules, plus writer
// priority: once a writer is waiting, no NEW reader starts before it.
#include "rw_tests.h"

int rw_waiting_writers(void);

// The priority check does not depend on timing:
//   1. Reader 1 goes in (the main thread).
//   2. A writer W arrives and must wait. We wait until rw_waiting_writers()
//      says 1, so we KNOW W is waiting.
//   3. A new reader R2 arrives. It must not get in yet.
//   4. Reader 1 leaves. W must get in before R2.
// Steps 3-4 compare the ORDER in which W and R2 got in. A correct solution
// always gives W first, however slow the machine is.
static void test_waiting_writer_goes_before_new_reader(void) {
    rw_init();
    atomic_store(&order_counter, 0);
    writer_got_in = reader_got_in = 0;
    start_read();                                         // step 1
    cvt_job *w = cvt_start(job_write_once, NULL);         // step 2
    int waiting = 0;
    for (int ms = 0; ms < 5000 && !waiting; ms++) {
        waiting = rw_waiting_writers() == 1;
        if (!waiting) sleep_ms(1);
    }
    if (!waiting) {
        CHECK(0, "a writer arrived while a reader was inside, but rw_waiting_writers() never "
                 "became 1. start_write() must do waiting_writers++ before it waits%s.",
              cvt_done(w) ? " (and the writer even got in although a reader was inside)" : "");
        end_read();
        cvt_wait(w, 5.0);
        return;
    }
    cvt_job *r2 = cvt_start(job_read_once, NULL);         // step 3
    sleep_ms(150);
    CHECK(!cvt_done(r2), "a NEW reader got in while a writer was waiting. With writer priority, "
                         "start_read() must also wait while waiting_writers > 0.");
    end_read();                                           // step 4
    int done = cvt_wait(w, 5.0) && cvt_wait(r2, 5.0);
    CHECK(done, "after reader 1 left, the waiting writer and the new reader did not both get in");
    if (done)
        CHECK(writer_got_in < reader_got_in,
              "the new reader got in (place %ld) before the writer that was already waiting (place %ld)",
              reader_got_in, writer_got_in);
    CHECK_EQ(rw_waiting_writers(), 0);
}

int main(void) {
    harness_watchdog(50);
    harness_hint("Start from track 4. Count waiting writers in start_write() (++ before the wait "
                 "loop, -- after it), make start_read() also wait while waiting_writers > 0, and "
                 "in end_write() wake a writer if one waits, otherwise broadcast to the readers.");
    RUN_TEST(test_readers_share);
    RUN_TEST(test_writer_waits_for_reader);
    RUN_TEST(test_others_wait_for_writer);
    RUN_TEST(test_waiting_writer_goes_before_new_reader);
    RUN_TEST(test_stress_invariant);
    return harness_report();
}
