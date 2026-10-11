# REQUIRES: mpi
# TIMEOUT: 120
if [ -z "${EX_SRC:-}" ] || [ ! -f "$EX_SRC" ]; then echo "  FAIL: cannot find ex4_mpi.c"; exit 1; fi
bin="$BUILD_DIR/ex4_mpi"
if ! log="$(mpicc $CFLAGS -I"$(dirname "$EX_SRC")" -o "$bin" "$EX_SRC" -lm 2>&1)"; then echo "$log" | sed 's/^/  | /'; echo "  FAIL: compile error (mpicc)"; exit 1; fi
[ -n "$log" ] && echo "$log" | sed 's/^/  | /'
fail=0
declare -A want=([100]=8 [100003]=1224 [2000000]=14871)
for args in "4 2000000 block" "4 2000000 cyclic" "4 2000000 dynamic" "3 100003 dynamic" "1 100 dynamic" "5 100 block" "3 100003 cyclic"; do
    set -- $args; np=$1; n=$2; mode=$3
    out="$(timeout -k 3 40 $MPIEXEC -n "$np" "$bin" "$n" "$mode" 2>&1)"; st=$?
    echo "  \$ mpiexec -n $np ex4_mpi $n $mode"; echo "$out" | sed 's/^/  | /' | head -6
    if [ $st -ne 0 ] || ! grep -q "^count = ${want[$n]}\$" <<<"$out"; then
        echo "  FAIL: -n $np, N=$n, $mode: expected 'count = ${want[$n]}' (exit code $st)"; fail=1; continue
    fi
    nw="$(sed -n 's/^work://p' <<<"$out" | wc -w)"
    [ "$nw" -eq "$np" ] || { echo "  FAIL: -n $np, $mode: the 'work:' line must list one number per rank"; fail=1; }
    if [ "$mode" = dynamic ] && [ "$np" -gt 1 ]; then
        w0="$(sed -n 's/^work: \([0-9]*\).*/\1/p' <<<"$out")"
        [ "$w0" = 0 ] || { echo "  FAIL: -n $np dynamic: the manager (rank 0) should do no items itself"; fail=1; }
    fi
done
[ $fail -eq 0 ] && echo "  PASS: all three distributions give the serial answer (compare the imbalance lines!)"
exit $fail
