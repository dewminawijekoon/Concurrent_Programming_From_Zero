// Lab track 4 - parallel merge sort with tasks and a depth cutoff   [Lab 03, Ex 4]
// BUILD: -fopenmp
//
// merge() and merge_sort_serial() are given. Sort with OpenMP tasks.
//
// PREDICT FIRST: with depth limit 3, how many tasks are created? With depth
//                limit 30 on 100,000 numbers, what happens to the run time?
//
// TODO 1: sort_rec(a, tmp, lo, hi, depth, depth_limit)
//   - if lo >= hi: return;
//   - if depth >= depth_limit: call merge_sort_serial and return;
//   - otherwise mid = lo + (hi - lo) / 2; sort the two halves in two
//     "#pragma omp task"s (with depth + 1), "#pragma omp taskwait",
//     then merge(a, tmp, lo, mid, hi).
// TODO 2: merge_sort_parallel - open a parallel region with
//   num_threads(nthreads) and let ONE thread ("#pragma omp single") call
//   sort_rec(a, tmp, 0, n - 1, 0, depth_limit).
// The test compares your result with qsort.
//
// Test:  make test M=04 T=lab4_mergesort
// Run:   make run  M=04 EX=lab4_mergesort ARGS="10000000 10"    (the lab's table)
#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Given: merge the sorted runs a[lo..mid] and a[mid+1..hi] using tmp, back into a.
void merge(int *a, int *tmp, int lo, int mid, int hi) {
    int i = lo, j = mid + 1, k = lo;
    while (i <= mid && j <= hi) tmp[k++] = (a[i] <= a[j]) ? a[i++] : a[j++];
    while (i <= mid) tmp[k++] = a[i++];
    while (j <= hi) tmp[k++] = a[j++];
    memcpy(a + lo, tmp + lo, sizeof(int) * (size_t)(hi - lo + 1));
}

// Given: serial merge sort of a[lo..hi] (both ends included).
void merge_sort_serial(int *a, int *tmp, int lo, int hi) {
    if (lo >= hi) return;
    int mid = lo + (hi - lo) / 2;
    merge_sort_serial(a, tmp, lo, mid);
    merge_sort_serial(a, tmp, mid + 1, hi);
    merge(a, tmp, lo, mid, hi);
}

static void sort_rec(int *a, int *tmp, int lo, int hi, int depth, int depth_limit) {
    // TODO 1
    (void)a; (void)tmp; (void)lo; (void)hi; (void)depth; (void)depth_limit;
}

void merge_sort_parallel(int *a, int *tmp, int n, int depth_limit, int nthreads) {
    // TODO 2
    (void)a; (void)tmp; (void)n; (void)depth_limit; (void)nthreads; (void)sort_rec;
}

#ifndef TESTING
int main(int argc, char *argv[]) {
    int n = argc > 1 ? atoi(argv[1]) : 10000000, limit = argc > 2 ? atoi(argv[2]) : 10;
    int *a = malloc(sizeof(int) * (size_t)n), *tmp = malloc(sizeof(int) * (size_t)n), *backup = malloc(sizeof(int) * (size_t)n);
    if (n < 1 || !a || !tmp || !backup) return 1;
    unsigned seed = 42;
    for (int i = 0; i < n; i++) backup[i] = rand_r(&seed);
    memcpy(a, backup, sizeof(int) * (size_t)n);
    double t0 = omp_get_wtime();
    merge_sort_serial(a, tmp, 0, n - 1);
    double ts = omp_get_wtime() - t0;
    printf("n = %d, depth limit %d\n\n| threads | serial (s) | parallel (s) | speedup |\n|--:|--:|--:|--:|\n", n, limit);
    for (int p = 1; p <= 8; p *= 2) {
        memcpy(a, backup, sizeof(int) * (size_t)n);
        t0 = omp_get_wtime();
        merge_sort_parallel(a, tmp, n, limit, p);
        double tp = omp_get_wtime() - t0;
        int ok = 1;
        for (int i = 1; i < n; i++) ok &= a[i - 1] <= a[i];
        printf("| %d | %.4f | %.4f | %.2f |%s\n", p, ts, tp, ts / tp, ok ? "" : "  NOT SORTED");
    }
    free(a); free(tmp); free(backup);
    return 0;
}
#endif
