# Checks the model of L06 slide 14 (two locks taken in opposite order).
# TIMEOUT: 60
code="$(sed 's/#.*//' "$EX_SRC")"
if [ "$(grep -cE '\.down\(\)' <<<"$code")" -lt 4 ] || [ "$(grep -c '^ *critical' <<<"$code")" -lt 2 ]; then
    echo "  FAIL: the model still has TODOs (each thread needs two down(), critical and two up())"; exit 1; fi
t2="$(sed -n '/thread T2/,$p' <<<"$code")"
first="$(grep -oE 'L[12]\.down' <<<"$t2" | head -1)"
[ "$first" = "L2.down" ] || { echo "  FAIL: this exercise is about the OPPOSITE order: T2 must lock L2 first"; exit 1; }
out="$(python3 "$ROOT/tools/interleave/interleave.py" "$EX_SRC" --verdict 2>&1)"
echo "$out" | sed 's/^/  | /' | head -20
grep -q 'deadlock=found' <<<"$out" || { echo "  FAIL: expected the explorer to find a deadlock"; exit 1; }
grep -q 'mutex=ok' <<<"$out" || { echo "  FAIL: both threads hold both locks in the critical section, so mutual exclusion should hold"; exit 1; }
echo "  PASS: T1 holds L1 and waits for L2 while T2 holds L2 and waits for L1. Name the 4 conditions."
exit 0
