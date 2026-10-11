// TIMEOUT: 60
// Tests for exercise 1: readers share, writers are alone, and a waiting
// writer is not overtaken by readers that arrive after it.
#include "classic_test_util.h"

void rw_init(void);
void rw_destroy(void);
void reader_lock(void);
void reader_unlock(void);
void writer_lock(void);
void writer_unlock(void);

static atomic_int r_in, w_in;   // set by helper threads once they are inside
static void *reader_thread(void *arg) {
    (void)arg;
    reader_lock();
    atomic_store(&r_in, 1);
    return NULL;                // stays inside: the test unlocks for it
}
static void *writer_thread(void *arg) {
    (void)arg;
    writer_lock();
    atomic_store(&w_in, 1);
    return NULL;
}

static void test_readers_share(void) {
    rw_init();
    atomic_store(&r_in, 0);
    reader_lock();
    pthread_t t;
    pthread_create(&t, NULL, reader_thread, NULL);
    int ok = wait_for_flag(&r_in, 2000);
    CHECK(ok, "a reader was inside and a second reader could not join: readers must share");
    reader_unlock();
    if (ok) reader_unlock();
    pthread_join(t, NULL);
    rw_destroy();
}

static void test_writer_excludes_everyone(void) {
    for (int kind = 0; kind < 2; kind++) {
        rw_init();
        atomic_store(&r_in, 0);
        atomic_store(&w_in, 0);
        writer_lock();
        pthread_t t;
        pthread_create(&t, NULL, kind == 0 ? reader_thread : writer_thread, NULL);
        sleep_ms(100);
        atomic_int *flag = kind == 0 ? &r_in : &w_in;
        CHECK(!atomic_load(flag), "a %s got in while a writer was inside",
              kind == 0 ? "reader" : "second writer");
        writer_unlock();
        CHECK(wait_for_flag(flag, 2000), "the writer left, but the waiting %s never got in",
              kind == 0 ? "reader" : "writer");
        pthread_join(t, NULL);
        if (kind == 0) reader_unlock();
        else writer_unlock();
        rw_destroy();
    }
}

static void test_writer_waits_for_reader(void) {
    rw_init();
    atomic_store(&w_in, 0);
    reader_lock();
    pthread_t t;
    pthread_create(&t, NULL, writer_thread, NULL);
    sleep_ms(100);
    CHECK(!atomic_load(&w_in), "a writer got in while a reader was inside");
    reader_unlock();
    CHECK(wait_for_flag(&w_in, 2000), "the reader left, but the waiting writer never got in");
    pthread_join(t, NULL);
    writer_unlock();
    rw_destroy();
}

// The key test. R1 inside; W arrives and waits; then R2 arrives.
// R2 must NOT overtake W, otherwise a stream of readers starves W.
static void test_new_reader_does_not_overtake_waiting_writer(void) {
    rw_init();
    atomic_store(&r_in, 0);
    atomic_store(&w_in, 0);
    reader_lock();                                    // R1 inside
    pthread_t w, r2;
    pthread_create(&w, NULL, writer_thread, NULL);    // W arrives
    sleep_ms(100);
    pthread_create(&r2, NULL, reader_thread, NULL);   // R2 arrives after W
    sleep_ms(100);
    int overtook = atomic_load(&r_in);
    CHECK(!overtook, "reader R2 arrived after writer W was waiting, but went in first. "
                     "If readers keep arriving like this, W waits forever (starvation)");
    reader_unlock();                                  // R1 leaves
    int w_got_in = wait_for_flag(&w_in, 2000);
    CHECK(w_got_in, "R1 left, but the waiting writer never got in");
    if (w_got_in) {
        sleep_ms(50);
        CHECK(!atomic_load(&r_in) || overtook, "R2 got in while the writer was inside");
        writer_unlock();
    }
    CHECK(wait_for_flag(&r_in, 2000), "the writer left, but reader R2 never got in");
    reader_unlock();                                  // R2 leaves
    pthread_join(r2, NULL);
    pthread_join(w, NULL);
    rw_destroy();
}

// Stress: 5 readers, 2 writers, random pauses; check the rules.
static atomic_int readers_inside, writers_inside, max_readers;
static atomic_long bad;
static long iters;
static void mixed(int id, void *ctx) {
    (void)ctx;
    for (long k = 0; k < iters; k++) {
        if (id < 5) {
            reader_lock();
            atomic_max(&max_readers, atomic_fetch_add(&readers_inside, 1) + 1);
            if (atomic_load(&writers_inside)) atomic_fetch_add(&bad, 1);
            harness_jitter();
            atomic_fetch_sub(&readers_inside, 1);
            reader_unlock();
        } else {
            writer_lock();
            if (atomic_fetch_add(&writers_inside, 1) != 0 || atomic_load(&readers_inside))
                atomic_fetch_add(&bad, 1);
            harness_jitter();
            atomic_fetch_sub(&writers_inside, 1);
            writer_unlock();
        }
        atomic_fetch_add(&progress, 1);
    }
}
static void test_stress(void) {
    rw_init();
    iters = harness_iters(20000);
    atomic_store(&bad, 0);
    progress_watch_start(5, "readers and writers are stuck (deadlock?)", NULL);
    harness_parallel(7, mixed, NULL);
    progress_watch_stop();
    rw_destroy();
    CHECK(atomic_load(&bad) == 0, "a writer shared the room with someone %ld times", atomic_load(&bad));
    if (atomic_load(&bad) == 0)
        printf("  (no violation observed in %ld entries; up to %d readers inside together)\n",
               7 * iters, atomic_load(&max_readers));
}

int main(void) {
    harness_watchdog(45);
    harness_hint("reader_lock: down(turnstile); up(turnstile); then the lightswitch. "
                 "writer_lock: down(turnstile); down(room_empty). "
                 "writer_unlock: up(turnstile); up(room_empty).");
    RUN_TEST(test_writer_waits_for_reader);
    RUN_TEST(test_writer_excludes_everyone);
    RUN_TEST(test_readers_share);
    RUN_TEST(test_new_reader_does_not_overtake_waiting_writer);
    RUN_TEST(test_stress);
    return harness_report();
}
