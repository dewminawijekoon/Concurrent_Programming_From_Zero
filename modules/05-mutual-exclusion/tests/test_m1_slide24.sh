# Checks the student's model of the L05 slide 24 algorithm.
# TIMEOUT: 60
out="$(python3 "$ROOT/tools/interleave/interleave.py" "$EX_SRC" --verdict 2>&1)"
echo "$out" | sed 's/^/  | /' | head -40
v="$(echo "$out" | grep '^VERDICT')"
code="$(sed 's/#.*//' "$EX_SRC")"     # ignore comments
if [ "$(grep -c '^ *critical' <<<"$code")" -lt 2 ] || [ "$(grep -c 'await' <<<"$code")" -lt 2 ]; then echo "  FAIL: the model still has TODOs (no await/critical yet)"; exit 1; fi
fail=0
for want in "mutex=violated" "deadlock=none" "blocked=none"; do
    grep -q "$want" <<<"$v" || { echo "  FAIL: expected $want - compare your model with the slide line by line"; fail=1; }
done
[ $fail -eq 0 ] && echo "  PASS: your model behaves like the slide's algorithm. Now write the answers to (i)-(iii) in words."
exit $fail
