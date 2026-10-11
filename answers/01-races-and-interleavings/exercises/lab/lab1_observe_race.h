// Interface for lab1_observe_race.c (shared by the exercise and its test)
#ifndef LAB1_OBSERVE_RACE_H
#define LAB1_OBSERVE_RACE_H

#define RACE_MAX_THREADS 64

struct race_stats {
    long min, max, range;   // smallest and largest result, and max - min
    double mean;            // average result
    double mean_error_pct;  // average of error_percent() over the runs
    int runs_with_loss;     // how many runs ended below the expected value
};

long race_once(int nthreads, long m);              // final counter of one racy run
double error_percent(long actual, long expected);  // 100 * (expected - actual) / expected
struct race_stats summarize_runs(const long results[], int runs, long expected);
#endif
