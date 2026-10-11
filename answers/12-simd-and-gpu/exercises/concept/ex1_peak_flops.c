// Exercise 1 (warm-up) - Peak GFLOPS, SIMD lanes, and the cost of copying   [L07 slides 4, 8, 14]
//
// PREDICT FIRST: the GPU on L07 slide 8 has 448 cores, but each core does only
//                1 floating-point operation per clock tick, at 1.15 GHz. The CPU
//                has 4 cores doing 4 operations per tick at 2.5 GHz. Which one
//                is faster at its best, and by roughly how many times?
//
// A "FLOP" is one floating-point operation, such as one multiply of two
// doubles. "GFLOPS" means billions (10^9) of them per second.
//
// Write the four small functions below. Each one is one line of arithmetic.
//
// Test:  make test M=12 T=ex1_peak_flops
// Run:   make run  M=12 EX=ex1_peak_flops
#include <stdio.h>

// Best possible speed in GFLOPS (L07 slide 8):
//   cores x (operations per clock tick per core) x (clock ticks per second, in GHz)
// GHz already means "billions of ticks per second", so the answer is in GFLOPS.
double peak_gflops(int cores, double flops_per_cycle_per_core, double ghz) {
    return cores * flops_per_cycle_per_core * ghz;
}

// How many numbers fit side by side in one vector register?
// e.g. a 256-bit AVX register holds 256 / 32 = 8 floats (a float is 32 bits).
int simd_lanes(int register_bits, int element_bits) {
    return register_bits / element_bits;
}

// Total time to use a GPU (L07 slide 14 warns about this):
//   time to copy the data over the PCI Express bus + time the kernel runs.
// bytes / (GB per second x 10^9) gives the copy time in seconds.
double offload_time(double kernel_s, double bytes_moved, double gb_per_s) {
    return kernel_s + bytes_moved / (gb_per_s * 1e9);
}

// Speedup of the GPU over the CPU once the copies are counted.
double offload_speedup(double cpu_s, double kernel_s, double bytes_moved, double gb_per_s) {
    return cpu_s / offload_time(kernel_s, bytes_moved, gb_per_s);
}

#ifndef TESTING
int main(void) {
    printf("CPU (4 cores x 4 flops x 2.5 GHz)     = %6.1f GFLOPS\n", peak_gflops(4, 4, 2.5));
    printf("GPU (448 cores x 1 flop x 1.15 GHz)   = %6.1f GFLOPS\n", peak_gflops(448, 1, 1.15));
    printf("floats in a 256-bit AVX register      = %d\n", simd_lanes(256, 32));
    printf("doubles in a 512-bit AVX-512 register = %d\n", simd_lanes(512, 64));
    // A job takes 0.5 s on the CPU and 0.01 s on the GPU, but needs 1.2 GB copied at 12 GB/s.
    printf("GPU speedup counting the copies       = %.2f (not 50!)\n",
           offload_speedup(0.5, 0.01, 1.2e9, 12));
    return 0;
}
#endif
