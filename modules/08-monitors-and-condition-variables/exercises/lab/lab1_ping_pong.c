// Lab track 1 - ping-pong with a condition variable      [Lab 02, Ex 1; L09 slides 9-17]
//
// PREDICT FIRST: the two threads below share ONE condition variable. When
//                ping finishes its turn and calls pthread_cond_signal(), who
//                can be woken up? Could ping wake itself? Does it matter?
//
// Two threads take turns. One says "ping", the other says "pong", and the
// output must be exactly  ping pong ping pong ...  (n of each), starting
// with ping, every single run.
//
// A shared variable `turn` says whose turn it is (PING or PONG). Each player:
//   1. locks the mutex,
//   2. waits on the condition variable WHILE it is not its turn,
//   3. calls pingpong_say(me, i)  (keep this inside the mutex!),
//   4. gives the turn to the other player and wakes it up,
//   5. unlocks the mutex.
// Use the mutex and the condition variable declared below. No busy-waiting.
//
// Test:  make test M=08 T=lab1_ping_pong
// Run:   make run  M=08 EX=lab1_ping_pong ARGS="5"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#define PING 0
#define PONG 1

int turn = PING;           // whose turn is it? protected by `mutex`
int rounds;                // n: how many times each player speaks
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

static void print_word(int who, int round) {
    (void)round;
    printf("%s\n", who == PING ? "ping" : "pong");
}

// The test replaces this with its own function, to record the order of the
// words. You don't need to change it. Call it while you hold the mutex.
void (*pingpong_say)(int who, int round) = print_word;

static void *player(void *arg) {
    int me = (int)(long)arg;      // PING or PONG
    int other = 1 - me;
    for (int i = 0; i < rounds; i++) {
        // TODO 1: lock the mutex
        // TODO 2: wait on `cond` while it is not my turn (turn != me)
        pingpong_say(me, i);
        // TODO 3: give the turn to the other player and wake it up
        // TODO 4: unlock the mutex
        (void)other;
    }
    return NULL;
}

// Play n rounds: start both players and wait until both have finished.
void pingpong(int n) {
    pthread_t t[2];
    rounds = n;
    turn = PING;
    pthread_create(&t[PING], NULL, player, (void *)(long)PING);
    pthread_create(&t[PONG], NULL, player, (void *)(long)PONG);
    pthread_join(t[PING], NULL);
    pthread_join(t[PONG], NULL);
}

#ifndef TESTING
int main(int argc, char **argv) {
    int n = argc > 1 ? atoi(argv[1]) : 5;
    pingpong(n);
    return 0;
}
#endif
