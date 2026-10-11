# Helpers shared by the MPI tests of module 14 (sourced, not run on its own).
mpi_build() { # mpi_build <output-binary>
    if [ -z "${EX_SRC:-}" ] || [ ! -f "$EX_SRC" ]; then echo "  FAIL: cannot find the exercise's .c file"; exit 1; fi
    local log; log="$(mpicc $CFLAGS -o "$1" "$EX_SRC" -lm 2>&1)"; local st=$?
    [ -n "$log" ] && echo "$log" | sed 's/^/  | /' | head -30
    if [ $st -ne 0 ]; then echo "  FAIL: compile error (mpicc)"; exit 1; fi
}
mpi_run() { # mpi_run <np> <seconds> <binary> [args...]: sets $OUT and $ST
    local np="$1" secs="$2"; shift 2
    OUT="$(timeout -k 3 "$secs" $MPIEXEC -n "$np" "$@" 2>&1)"; ST=$?
    echo "  \$ mpiexec -n $np $(basename "$1") ${*:2}   ($(grep -c '^ENTER' <<<"$OUT") critical-section entries)"
    grep -v '^ENTER\|^EXIT' <<<"$OUT" | sed 's/^/  | /' | head -15
    [ $ST -eq 124 ] || [ $ST -eq 137 ] && echo "  (killed after ${secs}s: the processes were stuck - a Recv with no matching Send?)"
}
# check_cs <np> <entries per rank>: no two ENTER..EXIT intervals overlap, and each rank entered k times
check_cs() {
    awk -v np="$1" -v k="$2" '
        $1 == "ENTER" { start[$2, ++ne[$2]] = $3 }
        $1 == "EXIT"  { stop[$2, ++nx[$2]] = $3 }
        END {
            bad = 0
            for (r = 0; r < np; r++) if (ne[r] != k || nx[r] != k) { printf "  FAIL: rank %d entered %d times (expected %d)\n", r, ne[r] + 0, k; bad = 1 }
            n = 0
            for (r = 0; r < np; r++) for (i = 1; i <= ne[r]; i++) { n++; s[n] = start[r, i]; e[n] = stop[r, i]; who[n] = r }
            for (i = 2; i <= n; i++) {                     # insertion sort by start time
                ts = s[i]; te = e[i]; tw = who[i]; j = i - 1
                while (j >= 1 && s[j] > ts) { s[j+1] = s[j]; e[j+1] = e[j]; who[j+1] = who[j]; j-- }
                s[j+1] = ts; e[j+1] = te; who[j+1] = tw
            }
            for (i = 2; i <= n; i++) if (s[i] < e[i-1]) { printf "  FAIL: rank %d entered while rank %d was still inside (mutual exclusion broken)\n", who[i], who[i-1]; bad = 1; break }
            exit bad
        }' <<<"$OUT"
}
