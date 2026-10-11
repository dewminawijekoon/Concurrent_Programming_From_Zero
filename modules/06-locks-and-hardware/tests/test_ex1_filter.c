// TIMEOUT: 60
#include "nlock.h"
#define LOCK_INIT(n) filter_init(n)
#define LOCK(i) filter_lock(i)
#define UNLOCK(i) filter_unlock(i)
#include "nthread_lock_tests.h"
int main(void) { return run_nthread_lock_tests(); }
