# Checks the student's model of the busy-wait global sum (L04 slides 21-26).
# TIMEOUT: 60
out="$(python3 "$ROOT/tools/interleave/interleave.py" "$EX_SRC" --outputs 2>&1)"
echo "$out" | sed 's/^/  | /' | head -20
code="$(sed 's/#.*//' "$EX_SRC")"     # ignore comments
if ! grep -q 'await' <<<"$code"; then echo "  FAIL: the model has no 'await' yet (the TODO is still open)"; exit 1; fi
n="$(echo "$out" | sed -n 's/.*Possible outputs when every thread finishes (\([0-9]*\)).*/\1/p')"
if [ "$n" = "1" ] && echo "$out" | grep -q '^ *1 3 6$'; then
    echo "  PASS: the only possible output is 1 3 6. Now answer (a)-(c) in words."
    exit 0
fi
echo "  FAIL: expected exactly one possible output, '1 3 6'. Check where you wait and where you pass the turn on."
exit 1
