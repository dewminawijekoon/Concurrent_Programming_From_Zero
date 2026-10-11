# REQUIRES: cuda
# TIMEOUT: 180
. "$(dirname "$0")/cuda_common.sh"
bin="$BUILD_DIR/ex6_cuda_sum"
cuda_build "$bin"
fail=0
for n in 1048699 1 255 256 100000; do
    cuda_run "ex6_cuda_sum $n" "$bin" "$n"
    grep -q "^atomic: OK" <<<"$OUT" || { echo "  FAIL: n=$n: sum_atomic gave the wrong total"; fail=1; }
    grep -q "^shared: OK" <<<"$OUT" || { echo "  FAIL: n=$n: sum_shared gave the wrong total"; fail=1; }
    [ $ST -eq 0 ] || { echo "  FAIL: n=$n: the program exited with code $ST"; fail=1; }
done
if [ $fail -eq 0 ]; then echo "  PASS: both kernels add up correctly (the 'racy' line is for information only)"; else
    echo "  hint: sum_shared needs __syncthreads() after filling part[] and after EVERY round, reached by all threads"; fi
exit $fail
