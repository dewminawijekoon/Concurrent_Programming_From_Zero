// break_it/if_not_while.c - why the wait must sit inside a WHILE loop.
//
// Run:   make run M=08 EX=if_not_while            (wait inside "if":    broken)
//        make run M=08 EX=if_not_while ARGS=while (wait inside "while": correct)
//
// Two consumers and one producer share a buffer (we only keep its `count`).
// Each round goes like this:
//   1. Both consumers find the buffer empty and go to sleep.
//   2. The producer adds ONE item and calls pthread_cond_broadcast(), which
//      wakes BOTH consumers.
//   3. The consumers get the mutex back one after the other. The first one
//      takes the item. The second one...
//        - with "if":    does not look again, and takes from an EMPTY buffer;
//        - with "while": looks again, sees count == 0, and goes back to sleep.
// The producer waits until both consumers are asleep before each round, so
// this happens in EVERY round, not just by bad luck. The same thing happens
// with pthread_cond_signal() when a third thread "barges in" and takes the
// item first (L09 slides 23-25), or after a spurious wake-up; those are just
// harder to show on demand.
#include <pthread.h>
#include <stdio.h>
#include <string.h>

#define ROUNDS 10000

static int count;               // items in the buffer
static int sleeping;            // consumers that went to sleep since the last broadcast
static int done;                // producer finished
static long took_from_empty;    // the bug: a take when count was already 0
static long taken;
static int use_while;
static pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t not_empty = PTHREAD_COND_INITIALIZER;
static pthread_cond_t both_asleep = PTHREAD_COND_INITIALIZER;

// Go to sleep until there is an item (or the producer is done).
static void wait_for_item(void) {
    if (use_while) {
        while (count == 0 && !done) {
            sleeping++;
            pthread_cond_signal(&both_asleep);  // tell the producer "I'm asleep"
            pthread_cond_wait(&not_empty, &m);
        }
    } else {
        if (count == 0 && !done) {
            sleeping++;
            pthread_cond_signal(&both_asleep);
            pthread_cond_wait(&not_empty, &m);
        }
    }
}

static void *consumer(void *arg) {
    (void)arg;
    pthread_mutex_lock(&m);
    for (;;) {
        wait_for_item();
        if (done && count == 0) break;
        if (count == 0) took_from_empty++;     // with "if" we get here: nothing to take!
        else count--, taken++;
    }
    pthread_mutex_unlock(&m);
    return NULL;
}

int main(int argc, char **argv) {
    use_while = argc > 1 && strcmp(argv[1], "while") == 0;
    pthread_t c[2];
    for (int i = 0; i < 2; i++) pthread_create(&c[i], NULL, consumer, NULL);
    pthread_mutex_lock(&m);
    for (int r = 0; r < ROUNDS; r++) {
        while (sleeping < 2) pthread_cond_wait(&both_asleep, &m);   // step 1
        sleeping = 0;
        count++;                                                    // step 2
        pthread_cond_broadcast(&not_empty);
    }
    while (sleeping < 2) pthread_cond_wait(&both_asleep, &m);       // last item taken
    done = 1;
    pthread_cond_broadcast(&not_empty);
    pthread_mutex_unlock(&m);
    for (int i = 0; i < 2; i++) pthread_join(c[i], NULL);

    printf("wait inside \"%s\": %d items produced, %ld taken properly, %ld takes from an EMPTY buffer\n",
           use_while ? "while" : "if", ROUNDS, taken, took_from_empty);
    if (took_from_empty)
        printf("=> In each round the broadcast woke both consumers. The first took the item.\n"
               "   The second never re-checked count, so it 'took' an item that was not there.\n"
               "   In a real buffer it would read an old slot and make count negative.\n"
               "   Try: ARGS=while\n");
    else
        printf("=> The second consumer re-checked count after waking, saw 0, and slept again.\n");
    return 0;
}
