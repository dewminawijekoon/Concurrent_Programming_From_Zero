// Exercise 2 (core) - parallel prefix sum (scan) in two passes          [EXTRA]
// BUILD: -fopenmp
//
// The prefix sum of 3 1 4 1 5 is 3 4 8 9 14: out[i] = in[0] + ... + in[i].
// The serial loop needs out[i-1] to compute out[i], so you cannot simply
// put "#pragma omp parallel for" on it (README section 8.4).
//
// PREDICT FIRST: with 4 threads and 20 numbers, how many additions does the
//                two-pass method below do in total, compared with 20 for
//                the serial loop? Is it still worth it?
//
// TODO: prefix_sum_parallel, in a "#pragma omp parallel num_threads(nthreads)"
//       region. Each thread takes the block first = id*n/p .. last-1
//       (p = omp_get_num_threads()) and:
//   Pass 1: computes the prefix sum of its OWN block into out[], starting
//           from 0, and stores its block total in offset[id + 1].
//   Wait:   "#pragma omp barrier", so that all block totals are ready.
//   Middle: ONE thread ("#pragma omp single") turns offset[] into running
//           totals: offset[t] += offset[t-1] for t = 1..p. Now offset[t] is
//           the sum of all blocks before block t. (single waits for
//           everyone at its end.)
//   Pass 2: adds offset[id] to every out[i] of its block.
// Example, 3 1 4 1 5 2 with 2 threads: pass 1 gives 3 4 8 | 1 6 8, totals
// 8 and 8, offsets 0 and 8, pass 2 gives 3 4 8 9 14 16.
//
// Test:  make test M=04 T=ex2_prefix_sum
// Run:   make run  M=04 EX=ex2_prefix_sum
#include <omp.h>
#include <stdio.h>
#include <stdlib.h>

// Given: the serial version.
void prefix_sum_serial(const long *in, long *out, long n) {
    long run = 0;
    for (long i = 0; i < n; i++) {
        run += in[i];
        out[i] = run;
    }
}

void prefix_sum_parallel(const long *in, long *out, long n, int nthreads) {
    long offset[257] = {0};           // nthreads is at most 256
    // TODO
    (void)in; (void)out; (void)n; (void)nthreads; (void)offset;
}

#ifndef TESTING
int main(void) {
    long n = 20;
    long in[20], out[20];
    for (long i = 0; i < n; i++) in[i] = i + 1;
    prefix_sum_parallel(in, out, n, 4);
    for (long i = 0; i < n; i++) printf("%ld ", out[i]);
    printf("\n(expected 1 3 6 10 ... 210)\n");
    return 0;
}
#endif
