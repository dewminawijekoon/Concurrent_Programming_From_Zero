// TIMEOUT: 60
// Tests for exercise 2: no two neighbours eat at once, and no deadlock.
#include "classic_test_util.h"

void dp_init(int count);
void dp_destroy(void);
void dp_pick_up(int i);
void dp_put_down(int i);

#define MAXP 16
static int nphil;
static long meals;
static atomic_int state[MAXP];        // 0 thinking, 1 holds its first fork, 2 eating
static atomic_int fork_user[MAXP];    // who is eating with fork f right now (-1 = nobody)
static atomic_long between_calls, clashes;
static atomic_int go;

// Called by the student's dp_pick_up between the two waits.
void dp_between_forks(int i) {
    atomic_fetch_add(&between_calls, 1);
    if (i >= 0 && i < MAXP) atomic_store(&state[i], 1);
    sleep_us(200 + harness_rand() % 300);   // hold the first fork for a moment
}

static void grab(int i, int f) {
    int expected = -1;
    if (!atomic_compare_exchange_strong(&fork_user[f], &expected, i)) {
        if (atomic_fetch_add(&clashes, 1) == 0)
            printf("    philosopher %d started eating with fork %d, but philosopher %d was eating with it\n",
                   i, f, expected);
    }
}
static void release(int i, int f) {
    int expected = i;
    atomic_compare_exchange_strong(&fork_user[f], &expected, -1);
}

static void philosopher(int i, void *ctx) {
    (void)ctx;
    while (!atomic_load(&go)) sched_yield();   // everyone starts at the same moment
    for (long m = 0; m < meals; m++) {
        dp_pick_up(i);
        atomic_store(&state[i], 2);
        grab(i, i);
        grab(i, (i + 1) % nphil);
        harness_jitter();
        release(i, i);
        release(i, (i + 1) % nphil);
        atomic_store(&state[i], 0);
        dp_put_down(i);
        atomic_fetch_add(&progress, 1);
        harness_jitter();
    }
}

static void report_stuck(void) {
    printf("          state of each philosopher:\n");
    int holding = 0;
    for (int i = 0; i < nphil; i++) {
        int s = atomic_load(&state[i]);
        printf("            philosopher %d: %s\n", i,
               s == 0 ? "thinking (or waiting for its first fork)"
                      : s == 1 ? "holds its first fork, waits for the second" : "eating");
        holding += s == 1;
    }
    if (holding == nphil)
        printf("          Every philosopher holds one fork and waits for the next one: a circle of\n"
               "          waiting, so nobody can ever eat. Make the circle impossible (L08 slide 28).\n");
}

static void *starter(void *arg) {
    (void)arg;
    sleep_ms(20);              // by now every philosopher thread exists
    atomic_store(&go, 1);      // release them all together
    return NULL;
}

static void run(int count) {
    nphil = count;
    meals = harness_iters(300);
    atomic_store(&go, 0);
    atomic_store(&between_calls, 0);
    atomic_store(&clashes, 0);
    for (int i = 0; i < MAXP; i++) {
        atomic_store(&state[i], 0);
        atomic_store(&fork_user[i], -1);
    }
    dp_init(count);
    progress_watch_start(3, "the philosophers are stuck: a deadlock", report_stuck);
    pthread_t st;
    pthread_create(&st, NULL, starter, NULL);
    harness_parallel(count, philosopher, NULL);
    pthread_join(st, NULL);
    progress_watch_stop();
    dp_destroy();
    CHECK(atomic_load(&clashes) == 0, "%d philosophers: two neighbours ate with the same fork %ld times",
          count, atomic_load(&clashes));
    CHECK(atomic_load(&between_calls) >= count * meals,
          "dp_between_forks() was called %ld times for %ld meals - keep the call between the two waits",
          atomic_load(&between_calls), count * meals);
    if (atomic_load(&clashes) == 0)
        printf("  (%d philosophers ate %ld meals each with no clash and no deadlock - "
               "observed, not proved; model m2 proves it for 3)\n", count, meals);
}

static void test_five_philosophers(void) { run(5); }
static void test_three_philosophers(void) { run(3); }
static void test_two_philosophers(void) { run(2); }

int main(void) {
    harness_watchdog(50);
    harness_hint("Let one philosopher pick up its forks in the other order, e.g. every philosopher "
                 "takes the LOWER-numbered of its two forks first.");
    RUN_TEST(test_five_philosophers);
    RUN_TEST(test_three_philosophers);
    RUN_TEST(test_two_philosophers);
    return harness_report();
}
