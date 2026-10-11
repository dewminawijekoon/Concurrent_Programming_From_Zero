# REQUIRES: mpi
# TIMEOUT: 90
. "$(dirname "$0")/mpi_common.sh"
bin="$BUILD_DIR/ex10_prefix_scan"
mpi_build "$bin"
fail=0
for args in "4 1000003" "3 10" "5 3" "1 100"; do
    set -- $args; np=$1; n=$2
    mpi_run "$np" 30 "$bin" "$n"
    [ $ST -eq 0 ] || { echo "  FAIL: -n $np: the program did not finish cleanly (exit code $ST)"; fail=1; continue; }
    want=$(( (n - 1) * n / 2 ))
    grep -q "^a\[n-1\] = $want\$" <<<"$OUT" || { echo "  FAIL: -n $np n=$n: expected 'a[n-1] = $want'"; fail=1; }
    grep -q "^mismatches: 0\$" <<<"$OUT" || { echo "  FAIL: -n $np n=$n: expected 'mismatches: 0'"; fail=1; }
done
[ $fail -eq 0 ] && echo "  PASS: the scan removed the dependence; every a[i] equals i(i+1)/2"
exit $fail
