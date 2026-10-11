// BUILD: -fopenmp
// REQUIRES: openmp
// TIMEOUT: 60
#include "omp_test.h"

enum sched { SCHED_STATIC, SCHED_DYNAMIC, SCHED_DYNAMIC64, SCHED_GUIDED };
void make_image(int *img, int h, int w, unsigned seed);
void blur_serial(const int *in, int *out, int h, int w);
void blur_parallel(const int *in, int *out, int h, int w, int nthreads, enum sched s);

static const char *names[] = {"static", "dynamic", "dynamic,64", "guided"};

static void check(int h, int w, int p, enum sched s) {
    long n = (long)h * w;
    int *in = malloc(sizeof(int) * n), *want = malloc(sizeof(int) * n), *got = malloc(sizeof(int) * n);
    make_image(in, h, w, 7u + (unsigned)h);
    for (long k = 0; k < n; k++) want[k] = got[k] = -1;      // untouched pixels stay -1
    blur_serial(in, want, h, w);
    blur_parallel(in, got, h, w, p, s);
    long bad = 0, first = -1;
    for (long k = 0; k < n; k++)
        if (got[k] != want[k]) { if (first < 0) first = k; bad++; }
    CHECK(bad == 0, "%d x %d image, %d threads, schedule %s: %ld pixels differ from the serial blur "
          "(first at row %ld, column %ld: %d instead of %d)", h, w, p, names[s], bad,
          first >= 0 ? first / w : 0, first >= 0 ? first % w : 0, first >= 0 ? got[first] : 0,
          first >= 0 ? want[first] : 0);
    free(in); free(want); free(got);
}

static void test_static(void) { check(300, 200, 4, SCHED_STATIC); check(7, 9, 8, SCHED_STATIC); }
static void test_dynamic(void) { check(300, 200, 4, SCHED_DYNAMIC); check(301, 50, 3, SCHED_DYNAMIC); }
static void test_dynamic64(void) { check(300, 200, 4, SCHED_DYNAMIC64); check(1000, 1000, 8, SCHED_DYNAMIC64); }
static void test_guided(void) { check(300, 200, 4, SCHED_GUIDED); check(1000, 1000, 8, SCHED_GUIDED); }
static void test_one_thread(void) { check(64, 64, 1, SCHED_STATIC); }

int main(void) {
    omp_test_skip_under_tsan();
    harness_watchdog(50);
    harness_hint("Share only the row loop (i). Declare j and sum INSIDE the loop body so each thread "
                 "has its own; a shared sum gives wrong pixels. Do not write the border pixels.");
    RUN_TEST(test_static);
    RUN_TEST(test_dynamic);
    RUN_TEST(test_dynamic64);
    RUN_TEST(test_guided);
    RUN_TEST(test_one_thread);
    return harness_report();
}
