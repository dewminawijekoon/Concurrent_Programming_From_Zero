# REQUIRES: cuda
# TIMEOUT: 180
. "$(dirname "$0")/cuda_common.sh"
bin="$BUILD_DIR/ex5_cuda_saxpy"
cuda_build "$bin"
fail=0
for n in 1000003 1 256 257; do
    cuda_run "ex5_cuda_saxpy $n" "$bin" "$n"
    if [ $ST -ne 0 ] || ! grep -q "^mismatches: 0 of $n\$" <<<"$OUT"; then
        echo "  FAIL: n=$n: the GPU result does not match the CPU result"
        fail=1
    fi
done
if [ $fail -eq 0 ]; then echo "  PASS: the GPU computed y = a*x + y correctly for all sizes"; else
    echo "  hint: check all five steps; blocks = (n + threads - 1) / threads; the kernel needs if (i < n)"; fi
exit $fail
