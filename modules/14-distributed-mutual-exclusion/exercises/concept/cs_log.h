// cs_log.h - used by the MPI exercises 5 and 6 to prove mutual exclusion. Do not change.
// Inside the critical section a process prints
//     ENTER <rank> <time in ns>   ...short pause...   EXIT <rank> <time in ns>
// using the machine-wide monotonic clock. The test sorts these intervals and
// checks that no two of them overlap. (All processes must run on one machine.)
#ifndef CS_LOG_H
#define CS_LOG_H
#include <stdio.h>
#include <time.h>
#include <unistd.h>

static long long cs_now_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (long long)ts.tv_sec * 1000000000LL + ts.tv_nsec;
}

static void critical_section(int rank) {
    printf("ENTER %d %lld\n", rank, cs_now_ns());
    fflush(stdout);
    usleep(2000);                      // stay inside long enough for an overlap to show
    printf("EXIT %d %lld\n", rank, cs_now_ns());
    fflush(stdout);
}
#endif
