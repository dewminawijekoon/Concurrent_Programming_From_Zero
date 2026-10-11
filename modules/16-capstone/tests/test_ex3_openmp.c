// BUILD: -fopenmp
// REQUIRES: openmp
// TIMEOUT: 90
#include <omp.h>
#include "capstone_checks.h"

long twins_omp(long N, int T, omp_sched_t kind, int chunk, long *work);

static void test_counts(void) {
    long Ns[] = {5, 100, 1000, 100003};
    int Ts[] = {1, 2, 3, 4};
    omp_sched_t kinds[] = {omp_sched_static, omp_sched_static, omp_sched_dynamic, omp_sched_guided};
    int chunks[] = {0, 1, 64, 0};
    const char *names[] = {"static", "static,1", "dynamic,64", "guided"};
    for (int i = 0; i < 4; i++) for (int k = 0; k < 4; k++) for (int s = 0; s < 4; s++) {
        long work[64] = {0};
        long c = twins_omp(Ns[i], Ts[k], kinds[s], chunks[s], work);
        if (!check_run(names[s], Ns[i], Ts[k], c, work)) return;
    }
    // static,1 is exactly the cyclic split: check one case per thread
    long work[64] = {0};
    twins_omp(100003, 4, omp_sched_static, 1, work);
    for (int t = 0; t < 4; t++)
        CHECK(work[t] == cyclic_work(100003, 4, t), "schedule(static,1): thread %d did %ld work, expected %ld (did you use schedule(runtime)?)",
              t, work[t], cyclic_work(100003, 4, t));
}

static void bench(void) {
    long N = 2000000, work[64];
    int T = 4;
    printf("\n  N = %ld, %d threads (times for information only)\n\n", N, T);
    printf("  | schedule | imbalance (max/avg work) | time (s) |\n  |---|--:|--:|\n");
    omp_sched_t kinds[] = {omp_sched_static, omp_sched_static, omp_sched_static, omp_sched_dynamic, omp_sched_dynamic, omp_sched_guided};
    int chunks[] = {0, 1, 3, 1, 256, 0};
    const char *names[] = {"static", "static,1", "static,3", "dynamic,1", "dynamic,256", "guided"};
    for (int s = 0; s < 6; s++) {
        double t0 = omp_get_wtime();
        twins_omp(N, T, kinds[s], chunks[s], work);
        printf("  | %s | %.2f | %.3f |\n", names[s], imb(work, T), omp_get_wtime() - t0);
    }
}

int main(void) {
#ifdef __SANITIZE_THREAD__
    // gcc's OpenMP runtime (libgomp) is not built for ThreadSanitizer, so TSan
    // cannot see OpenMP's own barriers and reports races that are not real.
    harness_skip("ThreadSanitizer gives false alarms inside gcc's OpenMP runtime; run make test instead");
#endif
    harness_watchdog(80);
    harness_hint("#pragma omp parallel num_threads(T) reduction(+:count) { long my_work = 0; #pragma omp for schedule(runtime) ...; work[omp_get_thread_num()] = my_work; }");
    RUN_TEST(test_counts);
    if (h_tests_failed == 0) bench();
    return harness_report();
}
