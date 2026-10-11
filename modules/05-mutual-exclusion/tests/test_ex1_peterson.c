// TIMEOUT: 60
#include "ex1_peterson.h"
#define LOCK_INIT() peterson_init()
#define LOCK(i) peterson_lock(i)
#define UNLOCK(i) peterson_unlock(i)
#include "two_thread_lock_tests.h"
int main(void) { return run_two_thread_lock_tests(); }
