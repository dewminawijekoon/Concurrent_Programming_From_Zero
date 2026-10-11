# TIMEOUT: 60
out="$(python3 "$ROOT/tools/interleave/interleave.py" "$EX_SRC" --verdict 2>&1)"
echo "$out" | sed 's/^/  | /' | head -30
code="$(sed 's/#.*//' "$EX_SRC")"     # ignore comments
grep -q 'victim *= *i' <<<"$code" && grep -q 'flag\[i\] *= *true' <<<"$code" && grep -q 'await' <<<"$code" \
    || { echo "  FAIL: model incomplete (need victim = i, flag[i] = true and the await)"; exit 1; }
vline=$(grep -n 'victim *= *i' <<<"$code" | head -1 | cut -d: -f1); fline=$(grep -n 'flag\[i\] *= *true' <<<"$code" | head -1 | cut -d: -f1)
[ "$vline" -lt "$fline" ] || { echo "  FAIL: this exercise is about the SWAPPED order: victim = i must come first"; exit 1; }
if grep -q 'mutex=violated' <<<"$out"; then echo "  PASS: the swapped version breaks mutual exclusion - study the schedule above"; exit 0; fi
echo "  FAIL: expected a mutual-exclusion violation. Did you put 'victim = i' BEFORE 'flag[i] = true'?"; exit 1
