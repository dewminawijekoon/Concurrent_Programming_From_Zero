// calc/interleavings.c - how many interleavings? (finished tool)      [L03 slides 33-34]
//
// Prints the lecture's formula with every number worked out, so you can
// check your hand calculations.
//
//   make run M=01 EX=interleavings ARGS="3 2"          n threads, m steps each
//   make run M=01 EX=interleavings ARGS="lens 1 2 3"   threads with different numbers of steps
//
// Formula (n threads, m atomic steps each):  (n*m)! / (m!)^n
// General (thread j has m_j steps):          (m_1 + ... + m_k)! / (m_1! * ... * m_k!)
//
// Why: there are T = total steps time slots. Thread 1 chooses which m_1 slots
// it uses, thread 2 chooses m_2 of the slots left, and so on. Each thread's
// own steps must stay in order, so choosing the slots fixes everything.
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef unsigned __int128 u128;
#define U128_MAX (~(u128)0)

// Print a 128-bit number in decimal.
static void print_u128(u128 x) {
    char buf[64];
    int i = 63;
    buf[i] = '\0';
    do {
        buf[--i] = (char)('0' + (int)(x % 10));
        x /= 10;
    } while (x);
    printf("%s", buf + i);
}

// x! exactly, or 0 if it does not fit in 128 bits (x <= 34 fits).
static u128 fact(int x) {
    u128 r = 1;
    for (int i = 2; i <= x; i++) {
        if (r > U128_MAX / (u128)i) return 0;
        r *= (u128)i;
    }
    return r;
}

// "a choose b" exactly, or 0 if it does not fit in 128 bits.
static u128 choose(int a, int b) {
    if (b < 0 || b > a) return 0;
    if (b > a - b) b = a - b;
    u128 r = 1;
    for (int i = 1; i <= b; i++) {
        u128 k = (u128)(a - b + i);
        if (r > U128_MAX / k) return 0;   // keep it simple: give up near the limit
        r = r * k / (u128)i;
    }
    return r;
}

static void print_number(int exact_ok, u128 exact, double log10v) {
    if (exact_ok) print_u128(exact);
    else printf("about %.3fe%d", pow(10.0, log10v - floor(log10v)), (int)floor(log10v));
}

static void explain(int k, const int len[]) {
    int total = 0;
    for (int j = 0; j < k; j++) total += len[j];

    // log10 of the answer, which works for any size
    double lg = lgamma(total + 1.0);
    for (int j = 0; j < k; j++) lg -= lgamma(len[j] + 1.0);
    double log10v = lg / log(10.0);

    // exact answer as a product of binomials, if it fits
    u128 exact = 1;
    int ok = 1, free_slots = total;
    for (int j = 0; j < k && ok; j++) {
        u128 w = choose(free_slots, len[j]);
        if (w == 0 || exact > U128_MAX / w) ok = 0;
        else exact *= w;
        free_slots -= len[j];
    }

    printf("Threads: %d, steps per thread:", k);
    for (int j = 0; j < k; j++) printf(" %d", len[j]);
    printf("   (total T = %d steps)\n\n", total);

    printf("Formula: T! / (");
    for (int j = 0; j < k; j++) printf("%s%d!", j ? " * " : "", len[j]);
    printf(")\n");

    u128 ft = fact(total);
    printf("  T! = %d! = ", total);
    print_number(ft != 0, ft, lgamma(total + 1.0) / log(10.0));
    printf("\n");
    for (int j = 0; j < k; j++) {
        if (j > 0 && len[j] == len[j - 1]) continue;   // print each distinct length once
        printf("  %d! = ", len[j]);
        u128 f = fact(len[j]);
        print_number(f != 0, f, lgamma(len[j] + 1.0) / log(10.0));
        printf("\n");
    }
    printf("\nNumber of interleavings = ");
    print_number(ok, exact, log10v);
    printf("\n");

    printf("\nThe same count built step by step (choose slots for each thread):\n");
    free_slots = total;
    for (int j = 0; j < k; j++) {
        u128 w = choose(free_slots, len[j]);
        printf("  thread %d picks %d of %d free slots: C(%d, %d) = ", j + 1, len[j], free_slots,
               free_slots, len[j]);
        if (w) print_u128(w); else printf("(too big to show)");
        printf("\n");
        free_slots -= len[j];
    }

    // Why testing every order is hopeless (L03 slide 32)
    double seconds = pow(10.0, log10v) * 1e-6;
    printf("\nIf you could test one interleaving per microsecond, testing them all would take ");
    if (seconds < 60) printf("%.3g seconds.\n", seconds);
    else if (seconds < 86400 * 365.0) printf("%.3g hours.\n", seconds / 3600);
    else printf("%.3g years.\n", seconds / (86400 * 365.0));
    printf("This is why we reason about ALL orders (proofs, tools/interleave on tiny models)\n"
           "instead of hoping that testing finds the bad one.\n");
}

int main(int argc, char **argv) {
    int len[64], k = 0;
    if (argc > 2 && strcmp(argv[1], "lens") == 0) {
        for (int i = 2; i < argc && k < 64; i++) len[k++] = atoi(argv[i]);
    } else {
        int n = argc > 1 ? atoi(argv[1]) : 3;
        int m = argc > 2 ? atoi(argv[2]) : 2;
        if (n < 1 || n > 64 || m < 0) {
            fprintf(stderr, "usage: interleavings N M   or   interleavings lens M1 M2 ...\n");
            return 2;
        }
        for (int j = 0; j < n; j++) len[k++] = m;
    }
    for (int j = 0; j < k; j++)
        if (len[j] < 0 || len[j] > 1000) {
            fprintf(stderr, "step counts must be between 0 and 1000\n");
            return 2;
        }
    explain(k, len);
    return 0;
}
