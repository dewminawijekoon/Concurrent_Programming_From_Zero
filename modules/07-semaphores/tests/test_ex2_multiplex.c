// TIMEOUT: 60
// Tests for exercise 2: a multiplex lets in at most N threads, and exactly N
// if N want in.
#include "sem_test_util.h"

void mplex_init(int n);
void mplex_destroy(void);
void mplex_enter(void);
void mplex_exit(void);
int mplex_try_enter(void);

// One thread on its own can take all N places (a semaphore is not owned by
// a thread, unlike a mutex). Then the next try must fail.
static void test_exactly_n_places(void) {
    mplex_init(3);
    mplex_enter();
    mplex_enter();
    mplex_enter();
    CHECK(mplex_try_enter() == 0, "with 3 of 3 places taken, mplex_try_enter() let a 4th thread in");
    mplex_exit();
    int got = mplex_try_enter();
    CHECK(got == 1, "one place was free again, but mplex_try_enter() said no");
    mplex_exit();
    if (got) mplex_exit();
    mplex_exit();
    mplex_destroy();
}

// N+1 threads arrive and stay inside for a while: only N may be inside.
static atomic_int inside, max_inside, release;
static void holder(int id, void *ctx) {
    (void)id;
    (void)ctx;
    mplex_enter();
    int now = atomic_fetch_add(&inside, 1) + 1;
    atomic_max(&max_inside, now);
    wait_for_flag(&release, 2000);   // stay inside until the test says go
    atomic_fetch_sub(&inside, 1);
    mplex_exit();
}
static void *releaser(void *arg) {
    (void)arg;
    sleep_ms(200);                   // give everybody time to get in (or block)
    atomic_store(&release, 1);
    return NULL;
}
static void test_extra_thread_waits(void) {
    atomic_store(&inside, 0);
    atomic_store(&max_inside, 0);
    atomic_store(&release, 0);
    mplex_init(3);
    pthread_t r;
    pthread_create(&r, NULL, releaser, NULL);
    harness_parallel(4, holder, NULL);
    pthread_join(r, NULL);
    mplex_destroy();
    CHECK(atomic_load(&max_inside) <= 3, "N = 3, but %d threads were inside at the same time",
          atomic_load(&max_inside));
    CHECK(atomic_load(&max_inside) == 3,
          "N = 3 and 4 threads wanted in, but at most %d were ever inside together - "
          "a multiplex must let N threads in at once",
          atomic_load(&max_inside));
}

// Many threads enter and leave quickly; count how many are inside.
static long iters;
static void stress_worker(int id, void *ctx) {
    (void)id;
    (void)ctx;
    for (long k = 0; k < iters; k++) {
        mplex_enter();
        int now = atomic_fetch_add(&inside, 1) + 1;
        atomic_max(&max_inside, now);
        harness_jitter();
        atomic_fetch_sub(&inside, 1);
        mplex_exit();
    }
}
static void test_stress(void) {
    atomic_store(&inside, 0);
    atomic_store(&max_inside, 0);
    iters = harness_iters(20000);
    mplex_init(3);
    harness_parallel(8, stress_worker, NULL);
    mplex_destroy();
    CHECK(atomic_load(&max_inside) <= 3, "N = 3, but %d threads were inside together",
          atomic_load(&max_inside));
    if (atomic_load(&max_inside) <= 3)
        printf("  (8 threads x %ld entries: never more than %d inside - no violation observed)\n",
               iters, atomic_load(&max_inside));
}

int main(void) {
    harness_watchdog(40);
    harness_hint("Start the semaphore at N. enter = sem_wait, exit = sem_post, "
                 "try_enter = (sem_trywait(...) == 0).");
    RUN_TEST(test_exactly_n_places);
    RUN_TEST(test_extra_thread_waits);
    RUN_TEST(test_stress);
    return harness_report();
}
