# REQUIRES: mpi
# TIMEOUT: 120
. "$(dirname "$0")/mpi_common.sh"
bin="$BUILD_DIR/ex6_tree_reduce_mpi"
mpi_build "$bin"
fail=0
for np in 1 2 4 5 8; do
    mpi_run "$np" 30 "$bin"
    [ $ST -eq 0 ] || { echo "  FAIL: -n $np: the program did not finish cleanly (exit code $ST)"; fail=1; continue; }
    want=$(( np * (np + 1) * (2 * np + 1) / 6 ))     # 1 + 4 + 9 + ... + np^2
    grep -q "^tree: $want reduce: $want\$" <<<"$OUT" || { echo "  FAIL: -n $np: expected 'tree: $want reduce: $want'"; fail=1; }
    if [ $((np & (np - 1))) -eq 0 ]; then
        n_ok="$(grep -c "^rank [0-9]* butterfly: $want " <<<"$OUT")"
        [ "$n_ok" -eq "$np" ] || { echo "  FAIL: -n $np: only $n_ok of $np ranks got the butterfly total $want"; fail=1; }
    fi
done
[ $fail -eq 0 ] && echo "  PASS: your tree sum and butterfly agree with MPI_Reduce and MPI_Allreduce"
exit $fail
