# Checks the student's fix of the "while, not if" monitor model.
# TIMEOUT: 60
out="$(python3 "$ROOT/tools/interleave/interleave.py" "$EX_SRC" --verdict 2>&1)"
echo "$out" | sed 's/^/  | /' | head -30
v="$(echo "$out" | grep '^VERDICT')"
code="$(sed 's/#.*//' "$EX_SRC")"     # ignore comments
if [ "$(grep -c 'assert count > 0' <<<"$code")" -lt 1 ] || [ "$(grep -c 'await' <<<"$code")" -lt 1 ]; then
    echo "  FAIL: keep the assert and the await in the consumer; only fix the waiting logic"; exit 1; fi
if [ "$(grep -c 'count = count + 1' <<<"$code")" -lt 2 ]; then
    echo "  FAIL: the producer must still put two items"; exit 1; fi
fail=0
for want in "assert=ok" "deadlock=none"; do
    grep -q "$want" <<<"$v" || { echo "  FAIL: expected $want. A consumer can still take from an empty buffer (or get stuck): look at the order of steps above"; fail=1; }
done
[ $fail -eq 0 ] && echo "  PASS: no order of steps lets a consumer take from an empty buffer. Now explain why in words."
exit $fail
