// pipeline_calc - worked solution for "how long does a pipeline take?" questions
//
//   make run M=11 EX=pipeline_calc ARGS="100 2 6 2"       100 items, stages of 2, 6 and 2 ms
//   make run M=11 EX=pipeline_calc ARGS="100 2 6/3 2"     the 6 ms stage runs on 3 workers
//
// Model (the one exam questions use):
//   - every item goes through every stage, in order;
//   - each stage works on one item at a time (or r items if it has r workers);
//   - passing an item to the next stage costs nothing, and queues never fill.
// Then the first item needs the sum of all stage times, and after that one
// item comes out every "bottleneck" time (the slowest stage). So
//        total  ~  (sum of stage times) + (n - 1) * (slowest stage time)
// With k equal stages of time t this is (k + n - 1) * t.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv) {
    if (argc < 3) {
        printf("usage: pipeline_calc N_ITEMS T1 T2 ... (stage times; write T/R for a stage "
               "with R workers)\n");
        return 2;
    }
    long n = atol(argv[1]);
    int k = argc - 2;
    double sum = 0, slowest = 0;
    printf("%ld items, %d stages\n\n", n, k);
    printf("| stage | time per item | workers | one item every |\n");
    printf("|------:|--------------:|--------:|---------------:|\n");
    for (int s = 0; s < k; s++) {
        double t = atof(argv[s + 2]);
        char *slash = strchr(argv[s + 2], '/');
        int r = slash ? atoi(slash + 1) : 1;
        if (r < 1) r = 1;
        double every = t / r;   // r workers finish r items per t, i.e. one per t / r
        printf("| %5d | %13g | %7d | %14g |\n", s + 1, t, r, every);
        sum += t;
        if (every > slowest) slowest = every;
    }
    double serial = n * sum;
    double piped = sum + (n - 1) * slowest;
    printf("\n1. One machine does every stage for every item:\n");
    printf("   n * (sum of stage times) = %ld * %g = %g\n", n, sum, serial);
    printf("2. Pipeline: the first item comes out after the sum of the stage times: %g\n", sum);
    printf("   After that, one item comes out every %g: the pace of the slowest stage\n", slowest);
    printf("   (the bottleneck: stage");
    for (int s = 0; s < k; s++) {
        double t = atof(argv[s + 2]);
        char *slash = strchr(argv[s + 2], '/');
        int r = slash && atoi(slash + 1) > 0 ? atoi(slash + 1) : 1;
        if (t / r == slowest) printf(" %d", s + 1);
    }
    printf(").\n");
    printf("   total = %g + (%ld - 1) * %g = %g\n", sum, n, slowest, piped);
    printf("3. Throughput (items per time unit) in the steady state = 1 / %g = %g\n", slowest,
           1.0 / slowest);
    printf("4. Speedup = %g / %g = %.2f   (it can never beat sum / slowest = %.2f)\n", serial,
           piped, serial / piped, sum / slowest);
    printf("\nTo go faster, make the bottleneck stage faster or give it more workers.\n");
    return 0;
}
