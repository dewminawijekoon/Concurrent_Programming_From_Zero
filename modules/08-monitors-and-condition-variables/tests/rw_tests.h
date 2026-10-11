// Shared tests for the reader-writer lab tracks 4 and 5.
// Both exercises provide rw_init, start_read, end_read, start_write, end_write.
#include "harness.h"
#include "trace.h"
#include "cv_helpers.h"

void rw_init(void);
void start_read(void);
void end_read(void);
void start_write(void);
void end_write(void);

// ---------------------------------------------------------------- small jobs
static atomic_long order_counter;     // hands out 1, 2, 3, ... in the order threads get in
static long writer_got_in, reader_got_in;

static void job_read_once(void *arg) {
    (void)arg;
    start_read();
    reader_got_in = atomic_fetch_add(&order_counter, 1) + 1;
    end_read();
}
static void job_write_once(void *arg) {
    (void)arg;
    start_write();
    writer_got_in = atomic_fetch_add(&order_counter, 1) + 1;
    end_write();
}

// While one reader is inside, a second reader must get in too.
static void test_readers_share(void) {
    rw_init();
    start_read();
    cvt_job *j = cvt_start(job_read_once, NULL);
    int ok = cvt_wait(j, 5.0);
    CHECK(ok, "a second reader could not start while one reader was inside. "
              "Readers must be able to read at the same time.");
    end_read();
    if (!ok) cvt_wait(j, 5.0);
}

// While a reader is inside, a writer must wait; it gets in after end_read().
static void test_writer_waits_for_reader(void) {
    rw_init();
    start_read();
    cvt_job *j = cvt_start(job_write_once, NULL);
    sleep_ms(150);
    CHECK(!cvt_done(j), "a writer got in while a reader was inside");
    end_read();
    CHECK(cvt_wait(j, 5.0), "the writer never got in after the last reader left "
                            "(does end_read() wake a writer when reader_count reaches 0?)");
}

// While a writer is inside, readers and other writers must wait.
static void test_others_wait_for_writer(void) {
    rw_init();
    start_write();
    cvt_job *r = cvt_start(job_read_once, NULL);
    cvt_job *w = cvt_start(job_write_once, NULL);
    sleep_ms(150);
    CHECK(!cvt_done(r), "a reader got in while a writer was inside");
    CHECK(!cvt_done(w), "a second writer got in while a writer was inside");
    end_write();
    CHECK(cvt_wait(r, 5.0) && cvt_wait(w, 5.0),
          "after end_write() the waiting reader and writer did not both get in "
          "(does end_write() wake the waiting threads?)");
}

// ---------------------------------------------------------------- stress test
#define RW_READERS 4
#define RW_WRITERS 2
static atomic_int readers_in, writers_in;
static atomic_long rw_bad;
static long rw_rounds;
static int shared_data;               // written by writers only, inside the lock

static void rw_worker(int id, void *ctx) {
    (void)ctx;
    int is_writer = id >= RW_READERS;
    for (long k = 0; k < rw_rounds; k++) {
        if (is_writer) {
            start_write();
            trace_event(id, "write-start", 0);
            int w = atomic_fetch_add(&writers_in, 1) + 1;
            if (w > 1 || atomic_load(&readers_in) > 0) atomic_fetch_add(&rw_bad, 1);
            int tmp = shared_data;
            harness_jitter();
            shared_data = tmp + 1;
            atomic_fetch_sub(&writers_in, 1);
            trace_event(id, "write-end", 0);
            end_write();
        } else {
            start_read();
            trace_event(id, "read-start", 0);
            atomic_fetch_add(&readers_in, 1);
            if (atomic_load(&writers_in) > 0) atomic_fetch_add(&rw_bad, 1);
            harness_jitter();
            atomic_fetch_sub(&readers_in, 1);
            trace_event(id, "read-end", 0);
            end_read();
        }
        if ((k & 3) == 0) sched_yield();
    }
}

// Replay the log: never a writer together with anyone else.
static void check_log(long *max_readers) {
    int r = 0, w = 0;
    *max_readers = 0;
    for (long i = 0; i < trace_len; i++) {
        const char *a = trace_log[i].action;
        if (strcmp(a, "read-start") == 0) r++;
        else if (strcmp(a, "read-end") == 0) r--;
        else if (strcmp(a, "write-start") == 0) w++;
        else if (strcmp(a, "write-end") == 0) w--;
        if (r > *max_readers) *max_readers = r;
        if (w > 1 || (w == 1 && r > 0)) {
            CHECK(0, "log event %ld: %d writer(s) and %d reader(s) inside at the same time", i, w, r);
            long from = i > 8 ? i - 8 : 0;
            printf("    events %ld..%ld:\n", from, i);
            for (long q = from; q <= i; q++)
                printf("      [%6ld] T%-3d %s\n", q, trace_log[q].thread, trace_log[q].action);
            return;
        }
    }
}

static void test_stress_invariant(void) {
    rw_init();
    trace_reset();
    shared_data = 0;
    atomic_store(&rw_bad, 0);
    long cap = TRACE_CAPACITY / (2 * (RW_READERS + RW_WRITERS));
    rw_rounds = harness_iters(4000);
    if (rw_rounds > cap) rw_rounds = cap;
    harness_parallel(RW_READERS + RW_WRITERS, rw_worker, NULL);
    CHECK(atomic_load(&rw_bad) == 0,
          "%ld times a thread got in while a writer was inside (or a writer got in with others inside)",
          atomic_load(&rw_bad));
    CHECK_EQ(shared_data, RW_WRITERS * rw_rounds);
    long max_readers;
    check_log(&max_readers);
    if (h_checks_failed == 0)
        printf("  (log of %ld events: never a writer with anyone else; up to %ld readers were "
               "inside together. No violation observed - a stress test cannot prove correctness)\n",
               trace_len, max_readers);
}
