// TIMEOUT: 30
#include "atomics.h"
#include "harness.h"

enum progress { UNANSWERED = 0, BLOCKING, OBSTRUCTION_FREE, LOCK_FREE, WAIT_FREE };
enum progress classify(char snippet);
int try_increment_once(shared_long *c);

// Questions to ask yourself when an answer is wrong (no answers given away).
static const char *why[] = {
    "A: what happens to the other threads if the thread holding m is paused?",
    "B: when a CAS fails, what does that tell you about the OTHER threads?",
    "C: how many steps does one call take, whatever the others do?",
    "D: when is the increment SURE to happen? (Lab 05 lists four classes and four snippets.)",
    "E: is a busy-waiting lock still a lock?",
    "F: can backoff turn 'some thread always wins' into 'every thread wins'?",
    "G: can another thread ever make my step fail or repeat?",
};

static void test_classification(void) {
    const enum progress want[] = {BLOCKING, LOCK_FREE, WAIT_FREE, OBSTRUCTION_FREE,
                                  BLOCKING, LOCK_FREE, WAIT_FREE};
    int wrong = 0;
    for (int i = 0; i < 7; i++) {
        enum progress got = classify((char)('A' + i));
        if (got == UNANSWERED) {
            CHECK(0, "snippet %c: no answer yet", 'A' + i);
            wrong++;
        } else if (got != want[i]) {
            CHECK(0, "snippet %c is not classified correctly. Think: %s", 'A' + i, why[i]);
            wrong++;
        }
    }
    if (!wrong) printf("    (all 7 correct - now write the 1-2 sentence justification for each)\n");
}

static void test_try_once_alone(void) {
    shared_long c = 5;
    CHECK(try_increment_once(&c) == 1, "alone, the CAS cannot fail: must return 1");
    CHECK_EQ(LOAD(c), 6);
}

static shared_long shared_c;
static shared_long successes, attempts_total;
static void try_worker(int id, void *ctx) {
    (void)id; (void)ctx;
    for (int i = 0; i < 100000; i++) {
        FETCH_ADD(attempts_total, 1);
        if (try_increment_once(&shared_c)) FETCH_ADD(successes, 1);
    }
}

static void test_try_once_contended(void) {
    STORE(shared_c, 0);
    STORE(successes, 0);
    STORE(attempts_total, 0);
    harness_parallel(4, try_worker, NULL);
    CHECK(LOAD(shared_c) == LOAD(successes), "every reported success must add exactly 1: counter "
                                             "= %ld, successes = %ld", LOAD(shared_c),
          LOAD(successes));
    printf("    (%ld of %ld attempts succeeded: the others gave up. That is why snippet D is "
           "not a correct counter.)\n", LOAD(successes), LOAD(attempts_total));
}

int main(void) {
    harness_watchdog(20);
    harness_hint("Re-read the four definitions at the top of the exercise. Ask: if one thread "
                 "is paused forever at the worst moment, can the others still finish?");
    RUN_TEST(test_classification);
    RUN_TEST(test_try_once_alone);
    RUN_TEST(test_try_once_contended);
    return harness_report();
}
