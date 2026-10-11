// Exercise 8 (from memory) - A whole CUDA program from a blank page   [L07 slides 22-26]
// REQUIRES: cuda
//
// PREDICT FIRST: list the five host-side steps of a CUDA program in order,
//                and write the kernel's first line (the global index) from memory.
//
// Without looking at ex5 or the README, write a complete program that:
//   - reads n from argv[1] (default 1000),
//   - fills int arrays a[i] = i and b[i] = 2 * i on the host,
//   - computes c[i] = a[i] + b[i] on the GPU (one thread per element,
//     256 threads per block, any n must work),
//   - checks every errors returned by the CUDA calls,
//   - copies c back and prints exactly these two lines:
//         c[last] = <value of c[n-1]>
//         mismatches: <number of i with c[i] != 3 * i>
//   - returns 0 if there were no mismatches.
// This is the exam's "outline a CUDA solution" question, done for real.
//
// Test:  make test M=12 T=ex8_cuda_vecadd_from_memory
// Run:   make run  M=12 EX=ex8_cuda_vecadd_from_memory ARGS=5000
#include <stdio.h>
#include <stdlib.h>

// TODO: the kernel

int main(int argc, char **argv) {
    // TODO
    (void)argc; (void)argv;
    return 1;
}
