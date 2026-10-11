// Shared by the module 04 tests.
#ifndef OMP_TEST_H
#define OMP_TEST_H
#include "harness.h"

// ThreadSanitizer reports false alarms for OpenMP programs: the OpenMP
// runtime library (libgomp) is not built for ThreadSanitizer, so TSan does
// not see the synchronization inside it. So under `make tsan` we skip.
static inline void omp_test_skip_under_tsan(void) {
#if defined(__SANITIZE_THREAD__)
    harness_skip("ThreadSanitizer gives false alarms with OpenMP (libgomp is not instrumented); "
                 "run `make test M=04` instead");
#endif
}

#endif
