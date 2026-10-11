# REQUIRES: mpi
# TIMEOUT: 90
. "$(dirname "$0")/mpi_common.sh"
bin="$BUILD_DIR/ex8_allreduce_stats"
mpi_build "$bin"
fail=0
for np in 4 3 1; do
    mpi_run "$np" 30 "$bin"
    [ $ST -eq 0 ] || { echo "  FAIL: -n $np: the program did not finish cleanly (exit code $ST)"; fail=1; continue; }
    want="$(awk -v n=$((1000 * np)) 'BEGIN { for (g = 0; g < n; g++) { v[g] = (g * 37) % 101; s += v[g] } m = s / n;
            for (g = 0; g < n; g++) q += (v[g] - m)^2; printf "mean = %.6f stddev = %.6f", m, sqrt(q / n) }')"
    n_ok="$(grep -c "^rank [0-9]*: $want\$" <<<"$OUT")"
    [ "$n_ok" -eq "$np" ] || { echo "  FAIL: -n $np: expected every rank to print '$want' ($n_ok of $np did)"; fail=1; }
done
[ $fail -eq 0 ] && echo "  PASS: every rank knows the global mean and standard deviation"
exit $fail
