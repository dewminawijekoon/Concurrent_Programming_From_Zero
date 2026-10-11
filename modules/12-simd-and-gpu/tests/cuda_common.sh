# Helpers shared by the CUDA tests (sourced, not run on its own).
# Set NVCC_FLAGS to pass extra flags, e.g. on Colab:  NVCC_FLAGS=-arch=sm_75 make test M=12
cuda_build() { # cuda_build <output-binary>
    if [ -z "${EX_SRC:-}" ] || [ ! -f "$EX_SRC" ]; then
        echo "  FAIL: cannot find the exercise's .cu file"; exit 1
    fi
    local log; log="$(nvcc -O2 ${NVCC_FLAGS:-} -o "$1" "$EX_SRC" 2>&1)"
    local st=$?
    [ -n "$log" ] && echo "$log" | sed 's/^/  | /' | head -30
    if [ $st -ne 0 ]; then echo "  FAIL: compile error (nvcc)"; exit 1; fi
}

# cuda_run <label> <binary> [args...]: run it, show its output, set $OUT and $ST
cuda_run() {
    local label="$1"; shift
    OUT="$(timeout 60 "$@" 2>&1)"; ST=$?
    echo "  \$ $label"
    echo "$OUT" | sed 's/^/  | /' | head -20
    if echo "$OUT" | grep -q "no CUDA-capable device\|driver version is insufficient"; then
        echo "  SKIP: nvcc works but no usable NVIDIA GPU was found"; exit 77
    fi
}
