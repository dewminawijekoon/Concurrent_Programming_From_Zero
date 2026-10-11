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
    if (n < 1) return 0.0;
    double sum = 0;
    for (int i = 0; i < n; i++) sum += x[i];
    return sum / n;
}

double sample_stddev(const double x[], int n) {
    if (n < 2) return 0.0;
    double m = mean(x, n), sq = 0;
    for (int i = 0; i < n; i++) sq += (x[i] - m) * (x[i] - m);
    return sqrt(sq / (n - 1));   // n - 1: we estimated the mean from the same data
}

void confidence_interval(const double x[], int n, double z, double *lo, double *hi) {
    double m = mean(x, n);
    double half = n > 0 ? z * sample_stddev(x, n) / sqrt((double)n) : 0.0;
    *lo = m - half;
    *hi = m + half;
}

long samples_needed(double z, double s, double r, double m) {
    double k = 100.0 * z * s / (r * m);
    double n = k * k;
    return (long)ceil(n - 1e-9);   // round up, but 96.0000000001 should not become 97
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
