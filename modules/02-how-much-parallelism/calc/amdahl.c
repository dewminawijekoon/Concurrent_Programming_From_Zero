// calc/amdahl.c - Amdahl's law and friends, with worked answers (finished tool)   [L02]
//
//   make run M=02 EX=amdahl ARGS="0.9"                 table for p = 0.9 (n = 1 .. 1024)
//   make run M=02 EX=amdahl ARGS="0.9 10"              one worked answer
//   make run M=02 EX=amdahl ARGS="compare 0.6 8 0.8 4" which option is faster?
//   make run M=02 EX=amdahl ARGS="need 0.9 5"          processors needed for a speedup of 5
//   make run M=02 EX=amdahl ARGS="estimate 2.5 4"      p from a measured speedup of 2.5 on 4
//   make run M=02 EX=amdahl ARGS="samples 20 5 5"      runs needed: mean 20, sd 5, +/-5% (95%)
//   make run M=02 EX=amdahl ARGS="samples 20 5 5 2.576"   ... at 99%
//   make run M=02 EX=amdahl ARGS="gustafson 0.9 10"    [EXTRA] scaled speedup
//
// p is the fraction of the RUNNING TIME (on one processor) that can run in
// parallel, not the fraction of the lines of code (L02 slide 6, speaker notes).
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static double amdahl(double p, double n) { return 1.0 / ((1.0 - p) + p / n); }

static void worked(double p, double n) {
    double serial = 1.0 - p, par = p / n, s = amdahl(p, n);
    printf("Amdahl's law: speedup = 1 / ((1 - p) + p / n)\n");
    printf("  p = %.4g (parallel fraction), n = %g processors\n", p, n);
    printf("  serial part   1 - p = %.4f   (takes the same time on any n)\n", serial);
    printf("  parallel part p / n = %.4g / %g = %.4f\n", p, n, par);
    printf("  time on n processors = %.4f + %.4f = %.4f   (time on 1 processor = 1)\n", serial, par,
           serial + par);
    printf("  speedup    = 1 / %.4f = %.2f\n", serial + par, s);
    printf("  efficiency = speedup / n = %.2f / %g = %.2f  (%.0f%% of the processors' time is useful)\n",
           s, n, s / n, 100 * s / n);
    if (p < 1)
        printf("  limit with unlimited processors = 1 / (1 - p) = 1 / %.4f = %.2f\n", serial,
               1.0 / serial);
}

static void table(double p) {
    printf("p = %.4g\n\n| n | speedup | efficiency |\n|---:|---:|---:|\n", p);
    double ns[] = {1, 2, 4, 8, 10, 16, 32, 64, 128, 1024};
    for (int i = 0; i < 10; i++)
        printf("| %g | %.2f | %.2f |\n", ns[i], amdahl(p, ns[i]), amdahl(p, ns[i]) / ns[i]);
    if (p < 1) printf("| unlimited | %.2f | 0 |\n", 1.0 / (1.0 - p));
    else printf("| unlimited | unlimited | 1 |\n");
}

static void compare(double p1, double n1, double p2, double n2) {
    printf("Option A: p = %.4g on %g processors\n", p1, n1);
    printf("  speedup = 1 / (%.4g + %.4g/%g) = 1 / %.4f = %.3f\n", 1 - p1, p1, n1,
           (1 - p1) + p1 / n1, amdahl(p1, n1));
    printf("Option B: p = %.4g on %g processors\n", p2, n2);
    printf("  speedup = 1 / (%.4g + %.4g/%g) = 1 / %.4f = %.3f\n", 1 - p2, p2, n2,
           (1 - p2) + p2 / n2, amdahl(p2, n2));
    double a = amdahl(p1, n1), b = amdahl(p2, n2);
    if (fabs(a - b) < 1e-9) printf("=> the same speedup.\n");
    else printf("=> option %s is faster (%.3f vs %.3f).\n", a > b ? "A" : "B", a > b ? a : b,
                a > b ? b : a);
    printf("   Limits with unlimited processors: A %.2f, B %.2f.\n",
           p1 < 1 ? 1 / (1 - p1) : INFINITY, p2 < 1 ? 1 / (1 - p2) : INFINITY);
    printf("   Efficiency: A %.2f, B %.2f.\n", a / n1, b / n2);
}

static void need(double p, double target) {
    printf("Smallest n with 1 / ((1 - p) + p / n) >= %.4g, for p = %.4g\n", target, p);
    if (p < 1 && target >= 1.0 / (1.0 - p) - 1e-9) {
        printf("  impossible: even unlimited processors give only 1 / (1 - p) = %.4g\n",
               1.0 / (1.0 - p));
        return;
    }
    if (target <= 1) { printf("  n = 1 (no speedup needed)\n"); return; }
    double x = p / (1.0 / target - (1.0 - p));
    printf("  (1 - p) + p / n <= 1 / target  ->  n >= p / (1/target - (1 - p))\n");
    printf("  n >= %.4g / (%.4g - %.4g) = %.4f  ->  n = %d\n", p, 1.0 / target, 1 - p, x,
           (int)ceil(x - 1e-9));
}

static void estimate(double s, double n) {
    printf("Measured speedup %.4g on %g processors. Solve s = 1 / ((1 - p) + p / n) for p:\n", s, n);
    if (n < 2) { printf("  needs n >= 2\n"); return; }
    double p = (1 - 1 / s) / (1 - 1 / n);
    printf("  p = (1 - 1/s) / (1 - 1/n) = (1 - %.4f) / (1 - %.4f) = %.4f\n", 1 / s, 1 / n, p);
    printf("  serial fraction 1 - p = %.4f\n", 1 - p);
    if (p > 0 && p < 1)
        printf("  prediction: 64 processors -> %.2f, unlimited -> %.2f\n", amdahl(p, 64), 1 / (1 - p));
}

static void samples(double mean, double sd, double r, double z) {
    printf("Runs needed for +/- %g%% accuracy, confidence z = %g (1.645 = 90%%, 1.96 = 95%%, 2.576 = 99%%)\n",
           r, z);
    double k = 100 * z * sd / (r * mean);
    printf("  n = (100 * z * s / (r * mean))^2 = (100 * %g * %g / (%g * %g))^2 = %.4f^2 = %.2f\n", z,
           sd, r, mean, k, k * k);
    printf("  round up: %ld runs\n", (long)ceil(k * k - 1e-9));
}

static void gustafson(double p, double n) {
    printf("[EXTRA] Gustafson's law: the problem grows with the machine.\n");
    printf("  p = parallel fraction of the time measured ON the n-processor run\n");
    printf("  scaled speedup = (1 - p) + p * n = %.4g + %.4g * %g = %.2f\n", 1 - p, p, n,
           (1 - p) + p * n);
    printf("  compare Amdahl (fixed problem size): %.2f\n", amdahl(p, n));
}

int main(int argc, char **argv) {
    if (argc >= 2 && strcmp(argv[1], "compare") == 0 && argc >= 6)
        compare(atof(argv[2]), atof(argv[3]), atof(argv[4]), atof(argv[5]));
    else if (argc >= 4 && strcmp(argv[1], "need") == 0)
        need(atof(argv[2]), atof(argv[3]));
    else if (argc >= 4 && strcmp(argv[1], "estimate") == 0)
        estimate(atof(argv[2]), atof(argv[3]));
    else if (argc >= 5 && strcmp(argv[1], "samples") == 0)
        samples(atof(argv[2]), atof(argv[3]), atof(argv[4]), argc >= 6 ? atof(argv[5]) : 1.96);
    else if (argc >= 4 && strcmp(argv[1], "gustafson") == 0)
        gustafson(atof(argv[2]), atof(argv[3]));
    else if (argc == 3)
        worked(atof(argv[1]), atof(argv[2]));
    else if (argc == 2 && atof(argv[1]) > 0)
        table(atof(argv[1]));
    else {
        printf("The four examples from L02 slides 6-9 (10 processors):\n\n");
        double ps[] = {0.6, 0.8, 0.9, 0.99};
        printf("| p | speedup | efficiency |\n|---:|---:|---:|\n");
        for (int i = 0; i < 4; i++)
            printf("| %.2f | %.2f | %.2f |\n", ps[i], amdahl(ps[i], 10), amdahl(ps[i], 10) / 10);
        printf("\nMore: see the comment at the top of calc/amdahl.c for all modes.\n");
    }
    return 0;
}
