// TIMEOUT: 60
// Tests for exercise 3: readers share the room, a writer is alone.
#include "sem_test_util.h"

void room_init(void);
void room_destroy(void);
void reader_enter(void);
void reader_exit(void);
void writer_enter(void);
void writer_exit(void);

static atomic_int got_in;   // set by a helper thread once it is inside

static void *reader_thread(void *arg) {
    (void)arg;
    reader_enter();
    atomic_store(&got_in, 1);
    return NULL;   // stays "inside": the test calls reader_exit() for it
}
static void *writer_thread(void *arg) {
    (void)arg;
    writer_enter();
    atomic_store(&got_in, 1);
    return NULL;
}

// A reader is inside; a second reader must be able to join.
static void test_readers_share(void) {
    room_init();
    atomic_store(&got_in, 0);
    reader_enter();
    pthread_t t;
    pthread_create(&t, NULL, reader_thread, NULL);
    int joined = wait_for_flag(&got_in, 2000);
    CHECK(joined, "a reader was inside and a second reader could not get in: readers must share");
    reader_exit();
    if (joined) reader_exit();   // the helper's turn to leave
    pthread_join(t, NULL);
    room_destroy();
}

// A reader is inside; a writer must wait until the reader leaves.
static void test_writer_waits_for_reader(void) {
    room_init();
    atomic_store(&got_in, 0);
    reader_enter();
    pthread_t t;
    pthread_create(&t, NULL, writer_thread, NULL);
    sleep_ms(100);
    CHECK(!atomic_load(&got_in), "a writer got in while a reader was inside");
    reader_exit();
    CHECK(wait_for_flag(&got_in, 2000), "the reader left, but the waiting writer never got in");
    pthread_join(t, NULL);
    writer_exit();
    room_destroy();
}

// A writer is inside; neither a reader nor a writer may join.
static void test_writer_is_alone(void) {
    for (int kind = 0; kind < 2; kind++) {
        room_init();
        atomic_store(&got_in, 0);
        writer_enter();
        pthread_t t;
        pthread_create(&t, NULL, kind == 0 ? reader_thread : writer_thread, NULL);
        sleep_ms(100);
        CHECK(!atomic_load(&got_in), "a %s got in while a writer was inside",
              kind == 0 ? "reader" : "second writer");
        writer_exit();
        CHECK(wait_for_flag(&got_in, 2000), "the writer left, but the waiting %s never got in",
              kind == 0 ? "reader" : "writer");
        pthread_join(t, NULL);
        if (kind == 0) reader_exit();
        else writer_exit();
        room_destroy();
    }
}

// The lightswitch lets new readers in while a writer waits (readers'
// preference). This is the behaviour of the pattern, and why writers can starve.
static void test_new_reader_overtakes_waiting_writer(void) {
    room_init();
    atomic_store(&got_in, 0);
    reader_enter();                                   // reader 1 inside
    pthread_t w, r2;
    pthread_create(&w, NULL, writer_thread, NULL);    // writer waits
    sleep_ms(50);
    pthread_create(&r2, NULL, reader_thread, NULL);   // reader 2 arrives
    // reader_thread sets got_in; the writer cannot be in, so got_in == reader 2
    int joined = wait_for_flag(&got_in, 2000);
    CHECK(joined, "with the lightswitch, a new reader should join the readers already inside, "
                  "even while a writer waits");
    reader_exit();
    if (joined) reader_exit();
    pthread_join(r2, NULL);
    pthread_join(w, NULL);   // the writer gets in once both readers are out
    writer_exit();
    room_destroy();
}

// Stress: 4 readers and 2 writers with random pauses; check the rules.
static atomic_int readers_in, writers_in, max_readers;
static atomic_long bad;
static long iters;
static void mixed(int id, void *ctx) {
    (void)ctx;
    for (long k = 0; k < iters; k++) {
        if (id < 4) {
            reader_enter();
            int r = atomic_fetch_add(&readers_in, 1) + 1;
            atomic_max(&max_readers, r);
            if (atomic_load(&writers_in) != 0) atomic_fetch_add(&bad, 1);
            harness_jitter();
            atomic_fetch_sub(&readers_in, 1);
            reader_exit();
        } else {
            writer_enter();
            int w = atomic_fetch_add(&writers_in, 1) + 1;
            if (w != 1 || atomic_load(&readers_in) != 0) atomic_fetch_add(&bad, 1);
            harness_jitter();
            atomic_fetch_sub(&writers_in, 1);
            writer_exit();
        }
    }
}
static void test_stress(void) {
    room_init();
    iters = harness_iters(20000);
    atomic_store(&bad, 0);
    harness_parallel(6, mixed, NULL);
    room_destroy();
    CHECK(atomic_load(&bad) == 0, "a writer shared the room with someone %ld times",
          atomic_load(&bad));
    if (atomic_load(&bad) == 0)
        printf("  (no violation observed in %ld entries; up to %d readers were inside together)\n",
               6 * iters, atomic_load(&max_readers));
}

int main(void) {
    harness_watchdog(40);
    harness_hint("ls_lock: mutex down; counter++; if it is now 1, down(room); mutex up. "
                 "ls_unlock: mutex down; counter--; if it is now 0, up(room); mutex up. "
                 "Readers use the lightswitch, writers use the room semaphore directly.");
    RUN_TEST(test_writer_waits_for_reader);
    RUN_TEST(test_writer_is_alone);
    RUN_TEST(test_readers_share);
    RUN_TEST(test_new_reader_overtakes_waiting_writer);
    RUN_TEST(test_stress);
    return harness_report();
}
