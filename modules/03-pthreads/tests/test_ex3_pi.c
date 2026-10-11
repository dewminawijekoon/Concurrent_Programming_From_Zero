// TIMEOUT: 60
#include <math.h>

#include "harness.h"

double pi_serial(long n);
double pi_mutex_each_term(long n, int nthreads);
double pi_mutex_local(long n, int nthreads);
double pi_busywait(long n, int nthreads);

static const int thread_counts[] = {1, 2, 3, 4, 7};

// The parallel sums add the same terms in a different order, so the last few
// digits may differ from the serial answer. 1e-9 is far bigger than that,
// and far smaller than any real mistake (a lost term changes pi by >1e-7).
static void check_version(const char *name, double (*fn)(long, int), long n) {
    double want = pi_serial(n);
    for (unsigned k = 0; k < sizeof thread_counts / sizeof *thread_counts; k++) {
        int p = thread_counts[k];
        double got = fn(n, p);
        CHECK(fabs(got - want) < 1e-9, "%s with %d threads and n=%ld: got %.12f, serial gives %.12f",
              name, p, n, got, want);
    }
    CHECK(fabs(fn(n, 4) - M_PI) < 4.0 / n + 1e-9, "%s: estimate is not close to pi", name);
}

static void test_mutex_each_term(void) { check_version("pi_mutex_each_term", pi_mutex_each_term, 200001); }
static void test_mutex_local(void) { check_version("pi_mutex_local", pi_mutex_local, 4000001); }
static void test_busywait(void) { check_version("pi_busywait", pi_busywait, 4000001); }

// Run the safe versions many times with tiny blocks: a missing lock or a
// wrong turn hand-over shows up as a wrong sum (or a timeout).
static void test_repeated(void) {
    int bad = 0;
    long runs = harness_iters(300);
    for (long r = 0; r < runs; r++) {
        if (fabs(pi_mutex_local(1001, 8) - pi_serial(1001)) > 1e-9) bad++;
        if (fabs(pi_busywait(1001, 8) - pi_serial(1001)) > 1e-9) bad++;
    }
    CHECK(bad == 0, "%d of %ld short runs gave a wrong sum", bad, 2 * runs);
    if (bad == 0)
        printf("    (no wrong sum observed in %ld runs; a stress test cannot prove there is no race,"
               " `make tsan M=03` checks for races)\n", 2 * runs);
}

int main(void) {
    harness_watchdog(50);
    harness_hint("Use a LOCAL variable for the block sum. If the first term of a block is odd, its "
                 "sign is minus. In the busy-wait version, wait for turn == id, add, then pass the "
                 "turn on with (id + 1) % n_threads; a timeout means the turn never reached someone.");
    RUN_TEST(test_mutex_local);
    RUN_TEST(test_busywait);
    RUN_TEST(test_mutex_each_term);
    RUN_TEST(test_repeated);
    return harness_report();
}
