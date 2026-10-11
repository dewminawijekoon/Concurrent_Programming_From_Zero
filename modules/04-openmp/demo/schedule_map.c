// demo/schedule_map.c - which thread runs which iteration, for each schedule
// BUILD: -fopenmp
//
// Run:   make run M=04 EX=schedule_map                 (20 iterations, 4 threads)
//        make run M=04 EX=schedule_map ARGS="100 3"
//
// Each row shows one schedule. Position i shows the thread that ran
// iteration i. static is the same on every run; dynamic and guided depend on
// which thread happens to be free first, so run it a few times.
// Every iteration does the same small amount of work here.
#include <omp.h>
#include <stdio.h>
#include <stdlib.h>

static void show(const char *name, omp_sched_t kind, int chunk, int n, int p) {
    int *owner = malloc(sizeof(int) * (size_t)n);
    omp_set_schedule(kind, chunk);
#pragma omp parallel for num_threads(p) schedule(runtime)
    for (int i = 0; i < n; i++) {
        volatile double x = 0;
        for (int k = 0; k < 20000; k++) x += k;      // a little work per iteration
        owner[i] = omp_get_thread_num();
    }
    printf("%-14s", name);
    for (int i = 0; i < n; i++) printf("%d", owner[i]);
    printf("\n");
    free(owner);
}

int main(int argc, char *argv[]) {
    int n = argc > 1 ? atoi(argv[1]) : 20, p = argc > 2 ? atoi(argv[2]) : 4;
    printf("%d iterations, %d threads (digit = thread that ran that iteration)\n\n", n, p);
    printf("%-14s", "iteration");
    for (int i = 0; i < n; i++) printf("%d", i % 10);
    printf("\n");
    show("static", omp_sched_static, 0, n, p);
    show("static, 2", omp_sched_static, 2, n, p);
    show("dynamic", omp_sched_dynamic, 1, n, p);
    show("dynamic, 3", omp_sched_dynamic, 3, n, p);
    show("guided", omp_sched_guided, 1, n, p);
    return 0;
}
