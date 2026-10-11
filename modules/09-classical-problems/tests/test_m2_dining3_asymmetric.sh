# Checks the student's model of three dining philosophers (asymmetric version).
# TIMEOUT: 60
out="$(python3 "$ROOT/tools/interleave/interleave.py" "$EX_SRC" --verdict 2>&1)"
echo "$out" | sed 's/^/  | /' | head -30
v="$(echo "$out" | grep '^VERDICT')"
code="$(sed 's/#.*//' "$EX_SRC")"     # ignore comments
if [ "$(grep -c '^ *critical' <<<"$code")" -lt 3 ] || [ "$(grep -cE 'down|wait|P\(' <<<"$code")" -lt 6 ]; then
    echo "  FAIL: the model still has TODOs (each philosopher needs two down() calls and a critical line)"; exit 1; fi
fail=0
for want in "mutex=ok" "deadlock=none"; do
    grep -q "$want" <<<"$v" || { echo "  FAIL: expected $want - check the order in which each philosopher picks up its forks"; fail=1; }
done
[ $fail -eq 0 ] && echo "  PASS: the explorer agrees with the expected result. Now answer the questions in the file."
exit $fail
