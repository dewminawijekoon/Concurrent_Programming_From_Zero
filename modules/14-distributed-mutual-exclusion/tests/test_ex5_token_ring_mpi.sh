# REQUIRES: mpi
# TIMEOUT: 90
. "$(dirname "$0")/mpi_common.sh"
bin="$BUILD_DIR/ex5_token_ring_mpi"
mpi_build "$bin"
fail=0
for np in 1 3 4; do
    mpi_run "$np" 30 "$bin"
    [ $ST -eq 0 ] || { echo "  FAIL: -n $np: the program did not finish cleanly (exit code $ST)"; fail=1; continue; }
    check_cs "$np" 5 || fail=1
done
[ $fail -eq 0 ] && echo "  PASS: no overlapping critical sections observed, every rank entered 5 times, and the ring shut down cleanly"
exit $fail
