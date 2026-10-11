# Checks the student's explorer model of the CAS-loop counter.
# TIMEOUT: 60
out="$(python3 "$ROOT/tools/interleave/interleave.py" "$EX_SRC" --outputs 2>&1)"
echo "$out" | sed 's/^/  | /' | head -20
code="$(sed 's/#.*//' "$EX_SRC")"     # ignore comments
if ! grep -q 'atomic:' <<<"$code"; then
    echo "  FAIL: the model has no atomic: block yet (the CAS must be one step)"; exit 1; fi
outs="$(echo "$out" | sed -n '/Possible outputs/,$p' | tail -n +2 | sed 's/^ *//' | grep -v '^$')"
if [ "$outs" != "2" ]; then
    echo "  FAIL: the only possible output must be 2, but the explorer found: $(echo $outs)"
    echo "        (1 means an increment was lost: is the compare-and-write really one step,"
    echo "         and do you retry after a failed CAS?)"
    exit 1
fi
echo "  PASS: every order of steps ends with counter = 2. Now try the give-up version (snippet D)."
exit 0
