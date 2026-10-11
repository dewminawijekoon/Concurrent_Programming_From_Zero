// twins.h - the capstone problem, shared by exercises 2-6. Do not change.
//
// Count the twin-prime pairs (p, p + 2) with p + 2 <= N, for example
// (3,5) (5,7) (11,13) (17,19) (29,31) ... There are 8 pairs with N = 100.
// The "items" are p = 0, 1, ..., N - 2. Testing item p costs a different
// amount of work for different p, which is what makes load balancing
// interesting. Work is counted in trial divisions, so it is the same on
// every machine and every run.
#ifndef TWINS_H
#define TWINS_H

// Trial division. Adds the number of divisibility tests to *work.
static inline int tw_is_prime(long n, long *work) {
    if (n < 2) return 0;
    *work += 1;
    if (n % 2 == 0) return n == 2;           // even numbers: one cheap test
    for (long d = 3; d * d <= n; d += 2) {   // odd numbers: up to sqrt(n) / 2 tests
        *work += 1;
        if (n % d == 0) return 0;
    }
    return 1;
}

// Item p: 1 if (p, p + 2) is a twin-prime pair, else 0.
static inline int tw_item(long p, long *work) {
    return tw_is_prime(p, work) && tw_is_prime(p + 2, work);
}

// Number of items for a given N (items are p = 0 .. N - 2).
static inline long tw_items(long N) { return N >= 2 ? N - 1 : 0; }

#endif
