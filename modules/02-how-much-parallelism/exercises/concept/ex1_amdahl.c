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

double speedup(double t1, double tp) {
    // TODO
    (void)t1; (void)tp;
    return 0.0;
}

double efficiency(double s, int n) {
    // TODO
    (void)s; (void)n;
    return 0.0;
}

double amdahl_speedup(double p, int n) {
    // TODO: 1 / ((1 - p) + p / n). Careful: p / n must be a double division.
    (void)p; (void)n;
    return 0.0;
}

double amdahl_limit(double p) {
    // TODO: 1 / (1 - p), or INFINITY (from math.h) if p >= 1.
    (void)p;
    return 0.0;
}

int processors_needed(double p, double target) {
    // TODO: return 1 if target <= 1; -1 if target >= amdahl_limit(p);
    //       otherwise the smallest n with amdahl_speedup(p, n) >= target - 1e-9.
    //       (A simple loop n = 1, 2, 3, ... is fine, or solve the formula for n.)
    (void)p; (void)target;
    return 0;
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
