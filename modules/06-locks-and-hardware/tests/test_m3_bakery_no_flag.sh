# Checks the flag-less Bakery model.
# TIMEOUT: 60
I="$ROOT/tools/interleave/interleave.py"
code="$(sed 's/#.*//' "$EX_SRC")"
grep -q 'await' <<<"$code" && grep -qE 'label\[i\] *= *m *\+ *1' <<<"$code" \
    || { echo "  FAIL: model incomplete (need m = label[1-i], label[i] = m + 1 and the await)"; exit 1; }
grep -q 'flag' <<<"$code" && { echo "  FAIL: this version has no flag[] at all"; exit 1; }
grep -q 'atomic' <<<"$code" && { echo "  FAIL: the tested version takes the label in TWO steps (no atomic block); try the atomic one separately"; exit 1; }
out="$(python3 "$I" "$EX_SRC" --bound 4 --verdict 2>&1)"
echo "$out" | sed 's/^/  | /' | head -24
if grep -q 'mutex=violated' <<<"$out"; then echo "  PASS: without the flag, a thread can miss a label that is about to be written. Explain the schedule above."; exit 0; fi
echo "  FAIL: expected a mutual-exclusion violation. Compare your lines with the pseudocode in the header."; exit 1
