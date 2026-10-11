// BUILD: -fopenmp
// REQUIRES: openmp
// TIMEOUT: 60
#include "harness.h"

int mandel_point(double cx, double cy, int max_iter);
void mandel_serial(int *img, int w, int h, int max_iter);
void mandel_static(int *img, int w, int h, int max_iter, int nthreads);
void mandel_dynamic(int *img, int w, int h, int max_iter, int nthreads);
void static_row_work(const int *img, int w, int h, int nthreads, long *work);

#define W 160
#define H 120
#define ITER 400

static int ref[W * H], img[W * H];

static int compare(const char *what, int nthreads) {
    for (int i = 0; i < W * H; i++)
        if (img[i] != ref[i]) {
            CHECK(0, "%s with %d threads: pixel (row %d, col %d) = %d, serial gives %d", what,
                  nthreads, i / W, i % W, img[i], ref[i]);
            return 0;
        }
    return 1;
}

static void test_static_matches_serial(void) {
    int threads[] = {1, 2, 3, 4, 7};
    for (int k = 0; k < 5; k++) {
        for (int i = 0; i < W * H; i++) img[i] = -1;
        mandel_static(img, W, H, ITER, threads[k]);
        if (!compare("mandel_static", threads[k])) return;
    }
}

static void test_dynamic_matches_serial(void) {
    int threads[] = {1, 2, 3, 4, 7};
    for (int k = 0; k < 5; k++) {
        for (int i = 0; i < W * H; i++) img[i] = -1;
        mandel_dynamic(img, W, H, ITER, threads[k]);
        if (!compare("mandel_dynamic", threads[k])) return;
    }
}

static void test_static_row_work(void) {
    int threads[] = {1, 3, 4, 7};
    for (int k = 0; k < 4; k++) {
        int p = threads[k];
        long work[8], want[8];
        for (int t = 0; t < 8; t++) work[t] = -1;
        static_row_work(ref, W, H, p, work);
        int q = H / p, rem = H % p, row = 0;
        for (int t = 0; t < p; t++) {
            want[t] = 0;
            for (int r = 0; r < q + (t < rem); r++, row++)
                for (int c = 0; c < W; c++) want[t] += ref[row * W + c];
            CHECK(work[t] == want[t], "static_row_work with %d threads: thread %d got %ld, "
                                      "expected %ld", p, t, work[t], want[t]);
        }
    }
    // The point of the exercise: the split is uneven.
    long work[4];
    static_row_work(ref, W, H, 4, work);
    printf("    work per thread with schedule(static), 4 threads: %ld %ld %ld %ld\n", work[0],
           work[1], work[2], work[3]);
}

int main(void) {
#ifdef __SANITIZE_THREAD__
    // GCC's OpenMP runtime (libgomp) is not built for ThreadSanitizer, so TSan
    // cannot see OpenMP's own barriers and reports races that are not real.
    harness_skip("ThreadSanitizer gives false alarms with OpenMP; run make test M=11 instead");
#endif
    harness_watchdog(50);
    harness_hint("Put the pragma on the row loop and declare the loop variables inside the "
                 "for statements so each thread has its own. For static_row_work, thread t's "
                 "rows come right after thread t-1's rows.");
    mandel_serial(ref, W, H, ITER);
    RUN_TEST(test_static_matches_serial);
    RUN_TEST(test_dynamic_matches_serial);
    RUN_TEST(test_static_row_work);
    return harness_report();
}
