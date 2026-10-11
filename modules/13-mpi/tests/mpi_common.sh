# Helpers shared by the MPI tests (sourced, not run on its own).
mpi_build() { # mpi_build <output-binary>
    if [ -z "${EX_SRC:-}" ] || [ ! -f "$EX_SRC" ]; then
        echo "  FAIL: cannot find the exercise's .c file"; exit 1
    fi
    local log; log="$(mpicc $CFLAGS -o "$1" "$EX_SRC" -lm 2>&1)"
    local st=$?
    [ -n "$log" ] && echo "$log" | sed 's/^/  | /' | head -30
    if [ $st -ne 0 ]; then echo "  FAIL: compile error (mpicc)"; exit 1; fi
}

# mpi_run <np> <seconds> <binary> [args...]: sets $OUT (stdout+stderr) and $ST (exit code)
mpi_run() {
    local np="$1" secs="$2"; shift 2
    OUT="$(timeout -k 3 "$secs" $MPIEXEC -n "$np" "$@" 2>&1)"; ST=$?
    echo "  \$ mpiexec -n $np $(basename "$1") ${*:2}"
    echo "$OUT" | sed 's/^/  | /' | head -25
    if [ $ST -eq 124 ] || [ $ST -eq 137 ]; then
        echo "  (killed after ${secs}s: the processes were stuck - deadlock? a Recv with no matching Send?)"
    fi
}
