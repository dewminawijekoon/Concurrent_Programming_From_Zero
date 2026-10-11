# REQUIRES: mpi
# TIMEOUT: 150
. "$(dirname "$0")/mpi_common.sh"
bin="$BUILD_DIR/ex9_exchange"
mpi_build "$bin"
fail=0
# 1,000,000 doubles = 8 MB per message: far too big for MPI to buffer, so a
# version where both partners send first would hang (and be killed).
for mode in ordered sendrecv nonblocking; do
    for np in 4 3; do
        mpi_run "$np" 25 "$bin" "$mode" 1000000
        partners=$(( np / 2 * 2 ))
        n_ok="$(grep -c '^rank [0-9]*: received OK$' <<<"$OUT")"
        if [ $ST -ne 0 ] || [ "$n_ok" -ne "$partners" ]; then
            echo "  FAIL: $mode with -n $np: $n_ok of $partners ranks received their partner's data (exit code $ST)"; fail=1
        fi
    done
done
[ $fail -eq 0 ] && echo "  PASS: all three exchanges work for 8 MB messages without deadlock"
exit $fail
