# REQUIRES: cuda
# TIMEOUT: 180
. "$(dirname "$0")/cuda_common.sh"
bin="$BUILD_DIR/ex7_cuda_matmul"
cuda_build "$bin"
fail=0
for n in 300 1 16 17 64; do
    cuda_run "ex7_cuda_matmul $n" "$bin" "$n"
    if [ $ST -ne 0 ] || ! grep -q "^mismatches: 0 of $((n * n))\$" <<<"$OUT"; then
        echo "  FAIL: N=$n: the GPU product does not match the CPU product"; fail=1
    fi
done
if [ $fail -eq 0 ]; then echo "  PASS: the GPU multiplies matrices of any size correctly"; else
    echo "  hint: grid = ((N+15)/16, (N+15)/16); in the kernel check row < N && col < N"; fi
exit $fail
