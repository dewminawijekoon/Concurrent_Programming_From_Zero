// Exercise 1 (warm-up) - speedup, efficiency and Amdahl's law     [L02 slides 4-11, 23-25]
//
// PREDICT FIRST: you make 90% of a program's running time parallel and run it
//                on 10 processors. Do you get a speedup of 9? 10? Less?
//                What if you had a million processors?
//
// Write these five small functions. Each one is a formula from the lesson.
//
//   speedup(t1, tp)        = t1 / tp        (time on 1 processor / time on p)
//   efficiency(s, n)       = s / n          (speedup per processor, between 0 and 1)
//   amdahl_speedup(p, n)   = 1 / ((1 - p) + p / n)
//                            p = fraction of the running time that can run in
//                            parallel (0.9 means 90%), n = number of processors
//   amdahl_limit(p)        = 1 / (1 - p)    (the speedup with "infinitely many"
//                            processors). Return INFINITY when p == 1.
//   processors_needed(p, target)
//                          = the SMALLEST whole number of processors n with
//                            amdahl_speedup(p, n) >= target.
//                            Return 1 if target <= 1.
//                            Return -1 if it is impossible (target >= the limit).
//                            Careful: floating-point numbers are not exact.
//                            1 / (0.05 + 0.95 / 19) should be exactly 10, but the
//                            computer may get 9.999999999999998. Allow a tiny
//                            tolerance, e.g. compare with target - 1e-9.
//
// Test:  make test M=02 T=ex1_amdahl
// Run:   make run  M=02 EX=ex1_amdahl ARGS="0.9 10"
//        (the finished calculator with worked answers is: make run M=02 EX=amdahl)
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "ex1_amdahl.h"

double speedup(double t1, double tp) { return t1 / tp; }

double efficiency(double s, int n) { return s / n; }

double amdahl_speedup(double p, int n) {
    // Time on 1 processor = 1 (normalised). The serial part (1 - p) takes the
    // same time; the parallel part p is shared by n processors: p / n.
    return 1.0 / ((1.0 - p) + p / n);
}

double amdahl_limit(double p) {
    if (p >= 1.0) return INFINITY;        // nothing serial: no limit
    return 1.0 / (1.0 - p);               // p / n -> 0 as n grows, only (1 - p) is left
}

int processors_needed(double p, double target) {
    if (target <= 1.0) return 1;
    if (target >= amdahl_limit(p) - 1e-9) return -1;   // can never reach it
    // Solve 1 / ((1 - p) + p / n) >= target for n:
    //   (1 - p) + p / n <= 1 / target   ->   n >= p / (1 / target - (1 - p))
    double n = p / (1.0 / target - (1.0 - p));
    int k = (int)ceil(n - 1e-9);          // tolerance: 18.999999999 counts as 19
    if (k < 1) k = 1;
    return k;
}

#ifndef TESTING
int main(int argc, char **argv) {
    double p = argc > 1 ? atof(argv[1]) : 0.9;
    int n = argc > 2 ? atoi(argv[2]) : 10;
    double s = amdahl_speedup(p, n);
    printf("p = %.2f, n = %d: speedup %.2f, efficiency %.2f, limit %.2f\n", p, n, s,
           efficiency(s, n), amdahl_limit(p));
    printf("processors needed for a speedup of 5: %d\n", processors_needed(p, 5.0));
    return 0;
}
#endif
