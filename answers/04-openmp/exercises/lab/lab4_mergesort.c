// ANSWER - Lab track 4: parallel merge sort with tasks and a depth cutoff   [Lab 03, Ex 4]
// BUILD: -fopenmp
//
// Test:  make verify-answers M=04
// Run:   make run M=04 EX=lab4_mergesort ANS=1 ARGS="10000000 10"
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
    if (lo >= hi) return;
    if (depth >= depth_limit) {               // deep enough: tasks would be too small
        merge_sort_serial(a, tmp, lo, hi);
        return;
    }
    int mid = lo + (hi - lo) / 2;
    // The two halves touch different parts of a and tmp, so they can run at
    // the same time without locks.
#pragma omp task
    sort_rec(a, tmp, lo, mid, depth + 1, depth_limit);
#pragma omp task
    sort_rec(a, tmp, mid + 1, hi, depth + 1, depth_limit);
#pragma omp taskwait                          // both halves must be sorted before merging
    merge(a, tmp, lo, mid, hi);
}

void merge_sort_parallel(int *a, int *tmp, int n, int depth_limit, int nthreads) {
#pragma omp parallel num_threads(nthreads)
    {
        // One thread starts the recursion; the rest of the team picks up
        // the tasks it creates. Without single, every thread would sort the
        // whole array at the same time, on the same memory: wrong results.
#pragma omp single
        sort_rec(a, tmp, 0, n - 1, 0, depth_limit);
    }
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
