// Interface for ex2_measuring.c (shared by the exercise and its test)
#ifndef EX2_MEASURING_H
#define EX2_MEASURING_H
double mean(const double x[], int n);
double sample_stddev(const double x[], int n);    // divides by n - 1; 0 if n < 2
void confidence_interval(const double x[], int n, double z, double *lo, double *hi);
long samples_needed(double z, double s, double r_percent, double m);   // rounded up
#endif
