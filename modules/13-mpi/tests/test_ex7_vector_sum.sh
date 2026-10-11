# REQUIRES: mpi
# TIMEOUT: 90
. "$(dirname "$0")/mpi_common.sh"
bin="$BUILD_DIR/ex7_vector_sum"
mpi_build "$bin"
fail=0
for args in "4 2.5" "3 -1" "1 2"; do
    set -- $args; np=$1; alpha=$2
    mpi_run "$np" 30 "$bin" "$alpha"
    [ $ST -eq 0 ] || { echo "  FAIL: -n $np: the program did not finish cleanly (exit code $ST)"; fail=1; continue; }
    want="$(awk -v a="$alpha" -v n=$((1000 * np)) 'BEGIN { printf "%.1f", a * (n - 1) + 1000 - (n - 1) }')"
    grep -q "^z\[n-1\] = $want\$" <<<"$OUT" || { echo "  FAIL: -n $np alpha=$alpha: expected 'z[n-1] = $want'"; fail=1; }
    grep -q "^mismatches: 0\$" <<<"$OUT" || { echo "  FAIL: -n $np alpha=$alpha: expected 'mismatches: 0'"; fail=1; }
done
[ $fail -eq 0 ] && echo "  PASS: Bcast, Scatter and Gather put every piece in the right place"
exit $fail
