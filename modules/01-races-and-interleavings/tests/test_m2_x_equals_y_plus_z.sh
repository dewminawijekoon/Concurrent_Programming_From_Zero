# Checks the student's load-store model of the L03 slide 41 program.
# TIMEOUT: 60
out="$(python3 "$ROOT/tools/interleave/interleave.py" "$EX_SRC" --outputs 2>&1)"
echo "$out" | sed 's/^/  | /' | head -20
code="$(sed 's/#.*//' "$EX_SRC")"     # ignore comments
if ! grep -q 'print' <<<"$code"; then echo "  FAIL: the model still has TODOs (no print yet)"; exit 1; fi
if grep -Eq 'x *= *y *\+ *z' <<<"$code"; then echo "  FAIL: x = y + z on one line is ONE step in the explorer. Split it into two loads and a store."; exit 1; fi
outs="$(sed -n '/Possible outputs/,$p' <<<"$out" | sed -n 's/^    //p' | sort | tr '\n' '|')"
want="0|1|2|3|"
if [ "$outs" = "$want" ]; then
    echo "  PASS: x can be 0, 1, 2 or 3. Now explain each value in words, and try loading z first."
    exit 0
fi
echo "  FAIL: expected the outputs 0, 1, 2 and 3, got: ${outs:-nothing}"
echo "        P1: a = y; b = z; x = a + b; print x.   P2: y = 1; z = 2."
exit 1
