// TIMEOUT: 60
void dekker_init(void);
void dekker_lock(int i);
void dekker_unlock(int i);
#define LOCK_INIT() dekker_init()
#define LOCK(i) dekker_lock(i)
#define UNLOCK(i) dekker_unlock(i)
#include "two_thread_lock_tests.h"
int main(void) { return run_two_thread_lock_tests(); }
