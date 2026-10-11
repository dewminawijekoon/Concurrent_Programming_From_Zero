# REQUIRES: mpi
# TIMEOUT: 90
. "$(dirname "$0")/mpi_common.sh"
bin="$BUILD_DIR/ex1_hello_rank"
mpi_build "$bin"
fail=0
for np in 1 4 6; do
    mpi_run "$np" 30 "$bin"
    [ $ST -eq 0 ] || { echo "  FAIL: -n $np: the program did not finish cleanly (exit code $ST)"; fail=1; continue; }
    n_direct="$(grep -c "^direct: hello from rank [0-9]* of $np\$" <<<"$OUT")"
    [ "$n_direct" -eq "$np" ] || { echo "  FAIL: -n $np: expected $np 'direct:' lines, found $n_direct"; fail=1; }
    got="$(grep '^ordered:' <<<"$OUT" | sed -n 's/^ordered: rank \([0-9]*\) of [0-9]* (pid [0-9]*)$/\1/p' | tr '\n' ' ')"
    want="$(seq 0 $((np - 1)) | tr '\n' ' ')"
    [ "$got" = "$want" ] || { echo "  FAIL: -n $np: 'ordered:' lines came in the order [$got], expected [$want]"; fail=1; }
    pids="$(grep '^ordered:' <<<"$OUT" | sed -n 's/.*(pid \([0-9]*\))$/\1/p' | sort -u | wc -l)"
    [ "$pids" -eq "$np" ] || { echo "  FAIL: -n $np: the 'ordered:' lines show $pids different pids; each line must carry its SENDER's pid"; fail=1; }
done
[ $fail -eq 0 ] && echo "  PASS: every rank says hello, and rank 0 prints the messages in rank order"
exit $fail
