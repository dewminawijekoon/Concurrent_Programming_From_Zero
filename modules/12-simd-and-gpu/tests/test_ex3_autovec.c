// BUILD: -O3
// TIMEOUT: 20
#include <math.h>
#include "harness.h"

void scale_add(float *restrict y, const float *restrict x, float a, int n);
void clamp_negatives(float *x, int n);
void running_sum(float *x, int n);
float sum_array(const float *x, int n);

#define N 1003

static void test_scale_add(void) {
    static float x[N], y[N];
    for (int i = 0; i < N; i++) { x[i] = (float)(i % 9); y[i] = (float)(i % 4); }
    scale_add(y, x, 2.0f, N);
    int wrong = 0;
    for (int i = 0; i < N; i++) wrong += y[i] != 2.0f * (float)(i % 9) + (float)(i % 4);
    CHECK(wrong == 0, "%d of %d results wrong: y[i] must become a*x[i] + y[i]", wrong, N);
}

static void test_clamp(void) {
    static float x[N];
    for (int i = 0; i < N; i++) x[i] = (float)(i % 7) - 3;   // -3 .. 3
    clamp_negatives(x, N);
    int wrong = 0;
    for (int i = 0; i < N; i++) {
        float want = (float)(i % 7) - 3;
        if (want < 0) want = 0;
        wrong += x[i] != want;
    }
    CHECK(wrong == 0, "%d of %d results wrong: negatives must become 0, the rest stay the same", wrong, N);
}

static void test_running_sum(void) {
    static float x[N];
    for (int i = 0; i < N; i++) x[i] = 1;
    running_sum(x, N);
    int wrong = 0;
    for (int i = 0; i < N; i++) wrong += x[i] != (float)(i + 1);
    CHECK(wrong == 0, "%d of %d results wrong: with all ones the running totals are 1, 2, 3, ...", wrong, N);
    float one[1] = {5};
    running_sum(one, 1);
    CHECK(one[0] == 5, "an array of length 1 must stay unchanged");
}

static void test_sum(void) {
    static float x[N];
    double exact = 0;
    for (int i = 0; i < N; i++) { x[i] = (float)(i % 11) * 0.5f; exact += x[i]; }
    CHECK_NEAR(sum_array(x, N), exact, 1e-3);
    CHECK_NEAR(sum_array(x, 0), 0, 0);
}

int main(void) {
    harness_watchdog(10);
    harness_hint("plain C loops, one element per step; the compiler does the vectorising");
    RUN_TEST(test_scale_add);
    RUN_TEST(test_clamp);
    RUN_TEST(test_running_sum);
    RUN_TEST(test_sum);
    return harness_report();
}
