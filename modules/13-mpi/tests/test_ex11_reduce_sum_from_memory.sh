# REQUIRES: mpi
# TIMEOUT: 90
. "$(dirname "$0")/mpi_common.sh"
bin="$BUILD_DIR/ex11_reduce_sum_from_memory"
mpi_build "$bin"
fail=0
for args in "4 1000000" "3 10" "5 3" "1 7" "6 1000001"; do
    set -- $args; np=$1; n=$2
    mpi_run "$np" 30 "$bin" "$n"
    want=$(( n * (n + 1) * (2 * n + 1) / 6 ))
    if [ $ST -ne 0 ] || ! grep -q "^sum = $want\$" <<<"$OUT" || [ "$(grep -c '^sum = ' <<<"$OUT")" -ne 1 ]; then
        echo "  FAIL: -n $np N=$n: expected exactly one line 'sum = $want' (printed by rank 0), exit code 0"; fail=1
    fi
done
[ $fail -eq 0 ] && echo "  PASS: your from-memory MPI program is correct for every process count"
exit $fail
