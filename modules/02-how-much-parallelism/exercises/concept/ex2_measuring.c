// Exercise 2 (core) - measure properly: mean, spread, confidence, sample size   [L02 slides 31-36]
//
// PREDICT FIRST: you time a program 5 times and get 18, 22, 20, 25 and 15
//                seconds. The mean is 20 s. Would you write "it takes 20 s"?
//                How sure are you? How many runs would you need to be 95%
//                sure the true mean is within +/- 5% (1 second) of 20 s,
//                if the standard deviation is 5 s?
//
// Write these functions:
//
//   mean(x, n)             the average: (x[0] + ... + x[n-1]) / n
//   sample_stddev(x, n)    the sample standard deviation s: how spread out
//                          the values are. Use n - 1 (not n):
//                              s = sqrt( sum of (x[i] - mean)^2  /  (n - 1) )
//                          Return 0 if n < 2.
//   confidence_interval(x, n, z, &lo, &hi)
//                          the range that contains the TRUE mean with the
//                          confidence that z stands for (z = 1.96 for 95%):
//                              mean - z * s / sqrt(n)   to   mean + z * s / sqrt(n)
//   samples_needed(z, s, r, m)
//                          how many runs give an accuracy of +/- r percent of
//                          the mean m, with confidence z (L02 slide 35):
//                              n = ( 100 * z * s / (r * m) )^2
//                          rounded UP to a whole number of runs.
//
// Test:  make test M=02 T=ex2_measuring
// Run:   make run  M=02 EX=ex2_measuring
#include <math.h>
#include <stdio.h>

#include "ex2_measuring.h"

double mean(const double x[], int n) {
    // TODO (return 0 if n < 1)
    (void)x; (void)n;
    return 0.0;
}

double sample_stddev(const double x[], int n) {
    // TODO: sqrt(sum of (x[i] - mean)^2 / (n - 1)); 0 if n < 2
    (void)x; (void)n;
    return 0.0;
}

void confidence_interval(const double x[], int n, double z, double *lo, double *hi) {
    // TODO: mean -/+ z * s / sqrt(n)
    (void)x; (void)n; (void)z;
    *lo = 0.0;
    *hi = 0.0;
}

long samples_needed(double z, double s, double r, double m) {
    // TODO: (100 * z * s / (r * m))^2, rounded UP (ceil). Allow a tiny tolerance
    //       so that 96.0000000001 does not become 97.
    (void)z; (void)s; (void)r; (void)m;
    return 0;
}

#ifndef TESTING
int main(void) {
    double t[] = {18, 22, 20, 25, 15};
    int n = 5;
    double lo, hi;
    confidence_interval(t, n, 1.96, &lo, &hi);
    printf("times: 18 22 20 25 15\n");
    printf("mean %.2f s, sample standard deviation %.3f s\n", mean(t, n), sample_stddev(t, n));
    printf("95%% confidence interval for the true mean: %.2f s to %.2f s\n", lo, hi);
    printf("runs needed for +/-5%% at 95%% (mean 20 s, s = 5 s): %ld\n",
           samples_needed(1.96, 5, 5, 20));
    return 0;
}
#endif
