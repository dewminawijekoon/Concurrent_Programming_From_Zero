// TIMEOUT: 60
// Checks lab4_rw_readers_pref: readers share, writers are alone, and (readers'
// preference) a new reader may still get in while a writer is waiting.
#include "rw_tests.h"

// Reader 1 is inside, a writer is waiting. In this version a NEW reader must
// still get in straight away: readers are only held back by an ACTIVE writer.
static void test_new_reader_passes_waiting_writer(void) {
    rw_init();
    start_read();                                 // reader 1 is inside
    cvt_job *w = cvt_start(job_write_once, NULL);
    sleep_ms(150);                                // give the writer time to start waiting
    cvt_job *r2 = cvt_start(job_read_once, NULL);
    int ok = cvt_wait(r2, 5.0);
    CHECK(ok, "a new reader could not get in while reader 1 was inside and a writer was waiting. "
              "In track 4 (readers' preference) readers wait only while a writer is ACTIVE. "
              "Waiting writers come in track 5.");
    end_read();
    CHECK(cvt_wait(w, 5.0), "the writer never got in after the readers left");
    if (!ok) cvt_wait(r2, 5.0);
}

int main(void) {
    harness_watchdog(50);
    harness_hint("Test and change reader_count/writer_active only while holding the mutex, wait in "
                 "while loops, and make sure the last reader out and every writer out wake the "
                 "threads that may now go in.");
    RUN_TEST(test_readers_share);
    RUN_TEST(test_writer_waits_for_reader);
    RUN_TEST(test_others_wait_for_writer);
    RUN_TEST(test_new_reader_passes_waiting_writer);
    RUN_TEST(test_stress_invariant);
    return harness_report();
}
