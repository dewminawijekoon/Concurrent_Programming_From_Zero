// Shared checks for the capstone tests (included by test files only).
#include "harness.h"
#include "bench.h"
#include "twins.h"

// Serial reference: count and per-item work, computed by the test itself.
__attribute__((unused)) static long ref_count(long N, long *total_work) {
    long c = 0, w = 0;
    for (long p = 0; p < tw_items(N); p++) c += tw_item(p, &w);
    *total_work = w;
    return c;
}
__attribute__((unused)) static long item_work(long p) { long w = 0; tw_item(p, &w); return w; }
__attribute__((unused)) static long block_work(long N, int T, int t) {
    long M = tw_items(N), q = M / T, extra = M % T;
    long n = q + (t < extra ? 1 : 0), first = t * q + (t < extra ? t : extra), w = 0;
    for (long p = first; p < first + n; p++) w += item_work(p);
    return w;
}
__attribute__((unused)) static long cyclic_work(long N, int T, int t) {
    long w = 0;
    for (long p = t; p < tw_items(N); p += T) w += item_work(p);
    return w;
}
__attribute__((unused)) static double imb(const long *w, int T) {
    long max = 0, sum = 0;
    for (int t = 0; t < T; t++) { sum += w[t]; if (w[t] > max) max = w[t]; }
    return sum ? (double)max * T / sum : 1.0;
}
// Checks a run: right count, and the work array adds up to the serial total.
__attribute__((unused)) static int check_run(const char *what, long N, int T, long got, const long *work) {
    long tw, want = ref_count(N, &tw), sum = 0;
    for (int t = 0; t < T; t++) sum += work[t];
    CHECK(got == want, "%s, N=%ld, %d threads: count %ld, expected %ld", what, N, T, got, want);
    CHECK(sum == tw, "%s, N=%ld, %d threads: work adds up to %ld, expected %ld (every item exactly once?)", what, N, T, sum, tw);
    return got == want && sum == tw;
}
