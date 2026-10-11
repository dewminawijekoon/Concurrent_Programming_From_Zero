# Checks the student's model of the slide 6 semaphore solution.
# TIMEOUT: 60
out="$(python3 "$ROOT/tools/interleave/interleave.py" "$EX_SRC" --verdict 2>&1)"
echo "$out" | sed 's/^/  | /' | head -30
v="$(echo "$out" | grep '^VERDICT')"
code="$(sed 's/#.*//' "$EX_SRC")"     # ignore comments
if [ "$(grep -c '^ *critical' <<<"$code")" -lt 2 ] || ! grep -qE '^ *sem' <<<"$code"; then
    echo "  FAIL: the model still has TODOs (no semaphore or no critical section yet)"; exit 1; fi
fail=0
for want in "mutex=ok" "deadlock=none" "starvation=found"; do
    grep -q "$want" <<<"$v" || { echo "  FAIL: expected $want - compare your model with slide 6"; fail=1; }
done
[ $fail -eq 0 ] && echo "  PASS: your model behaves like slide 6 with a busy-wait semaphore. Now answer (i)-(iii)."
exit $fail
