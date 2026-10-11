# REQUIRES: cuda
# TIMEOUT: 180
. "$(dirname "$0")/cuda_common.sh"
bin="$BUILD_DIR/ex8_cuda_vecadd_from_memory"
cuda_build "$bin"
fail=0
for n in 1000 1 256 1000001; do
    cuda_run "ex8_cuda_vecadd_from_memory $n" "$bin" "$n"
    want=$((3 * (n - 1)))
    grep -q "^c\[last\] = $want\$" <<<"$OUT" || { echo "  FAIL: n=$n: expected the line 'c[last] = $want'"; fail=1; }
    grep -q "^mismatches: 0\$" <<<"$OUT" || { echo "  FAIL: n=$n: expected the line 'mismatches: 0'"; fail=1; }
    [ $ST -eq 0 ] || { echo "  FAIL: n=$n: the program exited with code $ST (should be 0)"; fail=1; }
done
[ $fail -eq 0 ] && echo "  PASS: your from-memory CUDA program is correct"
exit $fail
