#!/usr/bin/env bash
# check.sh - report which parts of the course this machine can run.
ok()   { printf "  \e[32m[ok]\e[0m   %-28s %s\n" "$1" "$2"; }
miss() { printf "  \e[33m[--]\e[0m   %-28s %s\n" "$1" "$2"; }
CC="${CC:-gcc}"
tmp="$(mktemp -d)"; trap 'rm -rf "$tmp"' EXIT
echo "Toolchain check"
if command -v "$CC" >/dev/null; then ok "C compiler" "$($CC --version | head -1)"; else miss "C compiler" "install gcc (bash setup/install.sh)"; fi
printf '#include <pthread.h>\nvoid*f(void*a){return a;}int main(void){pthread_t t;pthread_create(&t,0,f,0);return pthread_join(t,0);}' > "$tmp/p.c"
if $CC -pthread "$tmp/p.c" -o "$tmp/p" 2>/dev/null && "$tmp/p"; then ok "Pthreads" "modules 01-11, 15, 16"; else miss "Pthreads" "needed for almost everything"; fi
printf '#include <semaphore.h>\nint main(void){sem_t s;return sem_init(&s,0,1);}' > "$tmp/s.c"
if $CC -pthread "$tmp/s.c" -o "$tmp/s" 2>/dev/null && "$tmp/s"; then ok "POSIX semaphores" "modules 07, 09"; else miss "POSIX semaphores" "unnamed semaphores missing (macOS?) - use the dev container"; fi
printf '#include <omp.h>\nint main(void){return omp_get_max_threads()<1;}' > "$tmp/o.c"
if $CC -fopenmp "$tmp/o.c" -o "$tmp/o" 2>/dev/null && "$tmp/o"; then ok "OpenMP" "modules 04, 11, 16"; else miss "OpenMP" "gcc -fopenmp failed (Apple clang has no OpenMP)"; fi
echo 'int main(void){return 0;}' > "$tmp/t.c"
if $CC -fsanitize=thread "$tmp/t.c" -o "$tmp/t" 2>/dev/null && { "$tmp/t" 2>/dev/null || setarch "$(uname -m)" -R "$tmp/t" 2>/dev/null; }; then ok "ThreadSanitizer" "make tsan M=.."; else miss "ThreadSanitizer" "optional: finds data races"; fi
if command -v mpicc >/dev/null && command -v mpiexec >/dev/null; then ok "MPI" "$(mpiexec --version 2>/dev/null | head -1)"; else miss "MPI" "modules 13, 14, 16 (bash setup/install.sh)"; fi
if command -v nvcc >/dev/null && nvidia-smi >/dev/null 2>&1; then ok "CUDA" "$(nvcc --version | tail -1)"; else miss "CUDA" "module 12: use Google Colab (see its README)"; fi
if grep -qw avx2 /proc/cpuinfo 2>/dev/null; then ok "AVX2 CPU" "module 12 SIMD part"; else miss "AVX2 CPU" "SIMD intrinsics exercise will be skipped"; fi
if command -v python3 >/dev/null; then ok "Python 3" "tools/interleave"; else miss "Python 3" "needed for tools/interleave"; fi
echo "  CPU cores: $(nproc 2>/dev/null || sysctl -n hw.ncpu)"
