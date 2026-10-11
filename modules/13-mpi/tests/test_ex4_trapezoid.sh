# REQUIRES: mpi
# TIMEOUT: 90
. "$(dirname "$0")/mpi_common.sh"
bin="$BUILD_DIR/ex4_trapezoid"
mpi_build "$bin"
fail=0
# Exact trapezoid answer for x^2 on [a,b] with n pieces: (b^3-a^3)/3 + (b-a) h^2 / 6.
check() { # check <np> <a> <b> <n>
    mpi_run "$1" 30 "$bin" "$2" "$3" "$4"
    [ $ST -eq 0 ] || { echo "  FAIL: -n $1: the program did not finish cleanly (exit code $ST)"; fail=1; return; }
    want="$(awk -v a="$2" -v b="$3" -v n="$4" 'BEGIN { h = (b - a) / n; printf "%.10f", (b^3 - a^3) / 3 + (b - a) * h * h / 6 }')"
    for kind in p2p reduce; do
        got="$(sed -n "s/^$kind: //p" <<<"$OUT" | head -1)"
        if [ -z "$got" ]; then echo "  FAIL: -n $1: no '$kind:' line printed"; fail=1; continue; fi
        ok="$(awk -v g="$got" -v w="$want" 'BEGIN { d = g - w; if (d < 0) d = -d; print (d < 1e-7) ? 1 : 0 }')"
        [ "$ok" = 1 ] || { echo "  FAIL: -n $1, a=$2 b=$3 n=$4: $kind gave $got, expected $want"; fail=1; }
    done
}
check 4 0 3 1024
check 3 0 3 1024      # 1024 is not a multiple of 3
check 1 0 3 1000
check 5 1 2 7         # fewer pieces per rank than you might expect
[ $fail -eq 0 ] && echo "  PASS: point-to-point and MPI_Reduce both give the right area for every process count"
exit $fail
