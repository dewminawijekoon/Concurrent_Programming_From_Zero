// Tests for the Bakery lock (ex2 and ex3): the n-thread tests plus a
// first-come-first-served check that uses the doorway and the waiting part
// separately.
#include "nlock.h"
#define LOCK_INIT(n) bakery_init(n)
#define LOCK(i) bakery_lock(i)
#define UNLOCK(i) bakery_unlock(i)
#include "nthread_lock_tests.h"

static shared_int entered;          // how many waiters have entered so far
static shared_int entry_order[2];   // who entered first, second

static void *waiter(void *arg) {
    int id = (int)(long)arg;
    bakery_wait(id);                // its doorway was already done for it
    int pos = FETCH_ADD(entered, 1);
    if (pos < 2) STORE(entry_order[pos], id);
    usleep(2000);                   // stay inside a little
    bakery_unlock(id);
    return NULL;
}

// Thread 0 holds the lock. Thread `first` finishes its doorway, THEN thread
// `second` does its doorway. Then `second` starts waiting 20 ms BEFORE
// `first` does. First-come-first-served means `first` must still go first.
static void fcfs_round(int first, int second) {
    STORE(entered, 0);
    bakery_lock(0);
    bakery_doorway(first);
    bakery_doorway(second);
    pthread_t t_second, t_first;
    pthread_create(&t_second, NULL, waiter, (void *)(long)second);
    usleep(20000);
    pthread_create(&t_first, NULL, waiter, (void *)(long)first);
    usleep(100000);
    int early = LOAD(entered);
    CHECK(early == 0, "%d thread(s) entered while thread 0 was still inside the critical section",
          early);
    bakery_unlock(0);
    pthread_join(t_second, NULL);
    pthread_join(t_first, NULL);
    if (early == 0)
        CHECK(LOAD(entry_order[0]) == first && LOAD(entry_order[1]) == second,
              "thread %d finished its doorway before thread %d started its doorway, so it "
              "must enter first; but the order was %d then %d", first, second,
              LOAD(entry_order[0]), LOAD(entry_order[1]));
}

static void test_first_come_first_served(void) {
    bakery_init(3);
    for (int k = 0; k < 5; k++) {     // make the labels non-zero first
        bakery_lock(k % 3);
        bakery_unlock(k % 3);
    }
    fcfs_round(1, 2);
    fcfs_round(2, 1);                 // the lower id must not always win
    fcfs_round(1, 2);
}

static int run_bakery_tests(void) {
    RUN_TEST(test_first_come_first_served);
    return run_nthread_lock_tests();
}
