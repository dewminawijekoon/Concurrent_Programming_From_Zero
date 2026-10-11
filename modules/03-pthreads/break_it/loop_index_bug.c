// break_it/loop_index_bug.c - the classic bug: passing &i to every thread.
//
// Run:   make run M=03 EX=loop_index_bug            (buggy version)
//        make run M=03 EX=loop_index_bug ARGS=fix   (each thread gets its own copy)
//
// main() starts 8 threads in a loop and passes &i, the ADDRESS of the loop
// counter. All 8 threads get the same address. Each thread reads i only when
// it actually starts running, and by then main may already have changed i.
// So several threads see the same number, some numbers are never seen, and
// a thread may even see 8 (the value after the loop ends).
#include <pthread.h>
#include <stdio.h>
#include <string.h>

#define N 8

static int seen[N + 1];               // how often each value was seen (index 8 = "after the loop")
static pthread_mutex_t seen_lock = PTHREAD_MUTEX_INITIALIZER;

static void *worker(void *arg) {
    int my_index = *(int *)arg;       // read through the pointer: whatever i is NOW
    pthread_mutex_lock(&seen_lock);
    if (my_index >= 0 && my_index <= N) seen[my_index]++;
    pthread_mutex_unlock(&seen_lock);
    return NULL;
}

int main(int argc, char *argv[]) {
    int fix = argc > 1 && strcmp(argv[1], "fix") == 0;
    int ids[N];                       // the fix: one separate int per thread
    pthread_t t[N];
    int rounds = 2000, rounds_with_bug = 0;
    for (int r = 0; r < rounds; r++) {
        memset(seen, 0, sizeof seen);
        int i;
        for (i = 0; i < N; i++) {
            ids[i] = i;
            pthread_create(&t[i], NULL, worker, fix ? (void *)&ids[i] : (void *)&i);
        }
        for (int k = 0; k < N; k++) pthread_join(t[k], NULL);
        int ok = seen[N] == 0;
        for (int k = 0; k < N; k++) ok &= seen[k] == 1;
        if (!ok) {
            if (rounds_with_bug == 0) {
                printf("first bad round: how many threads saw each index\n");
                for (int k = 0; k <= N; k++) printf("  index %d%s: %d threads\n", k,
                                                    k == N ? " (after the loop!)" : "", seen[k]);
            }
            rounds_with_bug++;
        }
    }
    printf("%s: %d of %d rounds gave some thread the wrong index\n",
           fix ? "with &ids[i]" : "with &i", rounds_with_bug, rounds);
    if (!fix)
        printf("Fix: give every thread its own variable (&ids[i], or a struct per thread),\n"
               "or pass the value itself: (void *)(intptr_t)i.\n");
    return 0;
}
