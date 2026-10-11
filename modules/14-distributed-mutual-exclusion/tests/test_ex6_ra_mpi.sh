# REQUIRES: mpi
# TIMEOUT: 120
. "$(dirname "$0")/mpi_common.sh"
bin="$BUILD_DIR/ex6_ra_mpi"
mpi_build "$bin"
fail=0
for np in 1 2 3 5; do
    for run in 1 2; do
        mpi_run "$np" 30 "$bin"
        [ $ST -eq 0 ] || { echo "  FAIL: -n $np: the program did not finish cleanly (exit code $ST)"; fail=1; continue; }
        check_cs "$np" 4 || fail=1
    done
done
[ $fail -eq 0 ] && echo "  PASS: no overlapping critical sections observed in 8 runs; every process entered 4 times and shut down cleanly"
exit $fail
