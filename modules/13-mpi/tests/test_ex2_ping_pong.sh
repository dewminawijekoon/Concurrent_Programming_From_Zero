# REQUIRES: mpi
# TIMEOUT: 90
. "$(dirname "$0")/mpi_common.sh"
bin="$BUILD_DIR/ex2_ping_pong"
mpi_build "$bin"
fail=0
for np in 2 4; do
    mpi_run "$np" 30 "$bin"
    [ $ST -eq 0 ] || { echo "  FAIL: -n $np: the program did not finish cleanly (exit code $ST)"; fail=1; continue; }
    grep -q "^final ball = 2000\$" <<<"$OUT" || { echo "  FAIL: -n $np: expected 'final ball = 2000' (1000 round trips, +1 at each hit)"; fail=1; }
    [ "$(grep -c '^final ball' <<<"$OUT")" -eq 1 ] || { echo "  FAIL: -n $np: only rank 0 should print the final ball"; fail=1; }
done
[ $fail -eq 0 ] && echo "  PASS: the ball went back and forth 1000 times (the timing table is for information only)"
exit $fail
