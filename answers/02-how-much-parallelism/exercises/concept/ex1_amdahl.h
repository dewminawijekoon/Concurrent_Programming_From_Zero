// Interface for ex1_amdahl.c (shared by the exercise and its test)
#ifndef EX1_AMDAHL_H
#define EX1_AMDAHL_H
double speedup(double t1, double tp);        // t1 / tp
double efficiency(double s, int n);          // s / n
double amdahl_speedup(double p, int n);      // 1 / ((1 - p) + p / n)
double amdahl_limit(double p);               // 1 / (1 - p), INFINITY if p == 1
int processors_needed(double p, double target);   // smallest n reaching target, -1 if impossible
#endif
