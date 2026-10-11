# Checks the model of a TAS spin lock on one core with priorities.
# TIMEOUT: 60
I="$ROOT/tools/interleave/interleave.py"
code="$(sed 's/#.*//' "$EX_SRC")"     # ignore comments
if [ "$(grep -c 'atomic' <<<"$code")" -lt 2 ] || [ "$(grep -c '^ *critical' <<<"$code")" -lt 2 ]; then
    echo "  FAIL: the model still has TODOs (each thread needs an atomic TestAndSet and a critical section)"; exit 1; fi
fair="$(python3 "$I" "$EX_SRC" --no-halt --verdict 2>&1)"
prio="$(python3 "$I" "$EX_SRC" --no-halt --priority H=2,L=1 --verdict 2>&1)"
echo "  with priorities (one core):"; echo "$prio" | sed 's/^/  | /' | head -30
vf="$(grep '^VERDICT' <<<"$fair")"; vp="$(grep '^VERDICT' <<<"$prio")"
fail=0
grep -q "mutex=ok" <<<"$vf" || { echo "  FAIL: with a fair scheduler the lock should keep mutual exclusion"; fail=1; }
grep -q "livelock=none" <<<"$vf" && grep -q "deadlock=none" <<<"$vf" || { echo "  FAIL: with a fair scheduler (several cores) both threads should finish; check acquire/release"; fail=1; }
grep -q "livelock=found" <<<"$vp" || { echo "  FAIL: with --priority H=2,L=1 the explorer should find H spinning forever. Does L set woken = 1 AFTER it takes the lock?"; fail=1; }
[ $fail -eq 0 ] && echo "  PASS: on one core, H spins forever and L (which holds the lock) never runs again. Now explain why in words."
exit $fail
