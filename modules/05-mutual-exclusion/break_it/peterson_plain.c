// break_it/peterson_plain.c - Peterson's lock written with plain `volatile int`.
//
// Run:   make run M=05 EX=peterson_plain            (plain volatile ints)
//        make run M=05 EX=peterson_plain ARGS=fence (with a memory fence)
//
// The algorithm is correct in the lecture model, yet on a real x86/ARM CPU
// this program LOSES increments, i.e. two threads end up in the critical
// section together. Why? Read the explanation printed at the end, or
// modules/05-mutual-exclusion/README.md, section "[EXTRA] Why your CPU breaks Peterson".
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <string.h>

#define ITERS 5000000

static volatile int flag[2];
static volatile int victim;
static long counter;          // protected by the lock (if the lock worked!)
static int use_fence;

static void lock(int i) {
    int j = 1 - i;
    flag[i] = 1;
    victim = i;
    // The CPU may do the READ of flag[j] below before the WRITES above are
    // visible to the other core (the writes wait in a small queue first).
    // A fence says: "make my earlier writes visible before any later read".
    if (use_fence) atomic_thread_fence(memory_order_seq_cst);
    while (flag[j] && victim == i) {
    }
}

static void unlock(int i) {
    if (use_fence) atomic_thread_fence(memory_order_seq_cst);
    flag[i] = 0;
}

static void *worker(void *arg) {
    int i = (int)(long)arg;
    for (int k = 0; k < ITERS; k++) {
        lock(i);
        counter++;
        unlock(i);
    }
    return NULL;
}

int main(int argc, char **argv) {
    use_fence = argc > 1 && strcmp(argv[1], "fence") == 0;
    pthread_t t[2];
    for (long i = 0; i < 2; i++) pthread_create(&t[i], NULL, worker, (void *)i);
    for (int i = 0; i < 2; i++) pthread_join(t[i], NULL);
    long expected = 2L * ITERS;
    printf("%s: counter = %ld, expected %ld, lost %ld increments\n",
           use_fence ? "with fence" : "plain volatile", counter, expected, expected - counter);
    if (counter != expected)
        printf("=> both threads were in the critical section together at least %ld times.\n"
               "   Why: a core's write to flag[i] reaches the other core a little late,\n"
               "   but the core already goes on and reads flag[j]. So each thread can see\n"
               "   the other's flag as 0, and both walk in. Try: ARGS=fence\n",
               expected - counter);
    else
        printf("=> no violation this run.%s\n",
               use_fence ? " The fence makes each write visible before the next read." :
                           " Run again; it depends on timing and CPU.");
    return 0;
}
