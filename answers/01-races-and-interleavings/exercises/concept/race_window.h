// race_window.h - make a race show up on (almost) every run.
//
// A race needs another thread to run at exactly the wrong moment, for
// example between reading a variable and writing it back. That window is
// only a few nanoseconds wide, so a racy program can look correct for a long
// time. race_window() gives up the CPU right there (sched_yield), which lets
// another thread run in the middle. The bug then shows up almost every time.
//
// Rule for the exercises: keep the race_window() calls where they are. A
// correct fix makes the window HARMLESS (nobody else can get in), it does not
// remove it. The tests count the calls to make sure they are still there.
#ifndef RACE_WINDOW_H
#define RACE_WINDOW_H
#include <sched.h>
#include <stdatomic.h>

// How many times race_window() ran. "weak" lets every file that includes this
// header share ONE counter (so the tests can read it).
__attribute__((weak)) atomic_long race_window_calls;

static inline void race_window(void) {
    atomic_fetch_add(&race_window_calls, 1);
    sched_yield();                      // "please let another thread run now"
}
#endif
