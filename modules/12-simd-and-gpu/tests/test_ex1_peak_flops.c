// TIMEOUT: 20
#include "harness.h"

double peak_gflops(int cores, double flops_per_cycle_per_core, double ghz);
int simd_lanes(int register_bits, int element_bits);
double offload_time(double kernel_s, double bytes_moved, double gb_per_s);
double offload_speedup(double cpu_s, double kernel_s, double bytes_moved, double gb_per_s);

static void test_slide8_example(void) {
    CHECK_NEAR(peak_gflops(4, 4, 2.5), 40.0, 1e-9);     // the CPU column of L07 slide 8
    CHECK_NEAR(peak_gflops(448, 1, 1.15), 515.2, 1e-9); // the GPU column (slide 8 rounds to 515)
    CHECK_NEAR(peak_gflops(1, 1, 1.0), 1.0, 1e-9);
}

static void test_lanes(void) {
    CHECK_EQ(simd_lanes(256, 32), 8);   // AVX: 8 floats
    CHECK_EQ(simd_lanes(256, 64), 4);   // AVX: 4 doubles
    CHECK_EQ(simd_lanes(128, 32), 4);   // SSE: 4 floats
    CHECK_EQ(simd_lanes(512, 32), 16);  // AVX-512: 16 floats
}

static void test_offload(void) {
    // 1.2 GB at 12 GB/s = 0.1 s of copying, plus 0.01 s of kernel.
    CHECK_NEAR(offload_time(0.01, 1.2e9, 12), 0.11, 1e-9);
    CHECK_NEAR(offload_speedup(0.5, 0.01, 1.2e9, 12), 0.5 / 0.11, 1e-6);
    // Nothing to copy: the speedup is just the ratio of the run times.
    CHECK_NEAR(offload_speedup(2.0, 0.01, 0, 10), 200.0, 1e-6);
}

int main(void) {
    harness_watchdog(10);
    harness_hint("peak = cores x flops per tick x GHz; lanes = register bits / element bits; "
                 "offload time = kernel time + bytes / (GB/s x 1e9)");
    RUN_TEST(test_slide8_example);
    RUN_TEST(test_lanes);
    RUN_TEST(test_offload);
    return harness_report();
}
