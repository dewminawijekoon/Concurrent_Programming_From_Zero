# REQUIRES: cuda
# TIMEOUT: 180
# Set NVCC_FLAGS for extra flags, e.g. on Colab:  NVCC_FLAGS=-arch=sm_75 make test M=16
if [ -z "${EX_SRC:-}" ] || [ ! -f "$EX_SRC" ]; then echo "  FAIL: cannot find ex5_cuda.cu"; exit 1; fi
bin="$BUILD_DIR/ex5_cuda"
if ! log="$(nvcc -O2 ${NVCC_FLAGS:-} -o "$bin" "$EX_SRC" 2>&1)"; then echo "$log" | sed 's/^/  | /'; echo "  FAIL: compile error (nvcc)"; exit 1; fi
fail=0
declare -A want=([5]=1 [100]=8 [100003]=1224 [1000000]=8169)
for n in 5 100 100003 1000000; do
    out="$(timeout 60 "$bin" "$n" 2>&1)"; st=$?
    echo "  \$ ex5_cuda $n"; echo "$out" | sed 's/^/  | /' | head -5
    if grep -q "no CUDA-capable device\|driver version is insufficient" <<<"$out"; then echo "  SKIP: no usable NVIDIA GPU"; exit 77; fi
    if [ $st -ne 0 ] || ! grep -q "GPU count = ${want[$n]}," <<<"$out"; then echo "  FAIL: N=$n: the GPU count should be ${want[$n]}"; fail=1; fi
done
[ $fail -eq 0 ] && echo "  PASS: the GPU agrees with the serial answer"
exit $fail
