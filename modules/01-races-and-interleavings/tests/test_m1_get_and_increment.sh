# Checks the student's model of getAndIncrement() split into a read and a write.
# TIMEOUT: 60
out="$(python3 "$ROOT/tools/interleave/interleave.py" "$EX_SRC" --outputs 2>&1)"
echo "$out" | sed 's/^/  | /' | head -20
code="$(sed 's/#.*//' "$EX_SRC")"     # ignore comments
if ! grep -q 'print' <<<"$code"; then echo "  FAIL: the model still has TODOs (no print yet)"; exit 1; fi
if grep -q 'atomic' <<<"$code"; then echo "  FAIL: do not use atomic: here. The point is to split the read and the write into separate steps."; exit 1; fi
outs="$(sed -n '/Possible outputs/,$p' <<<"$out" | sed -n 's/^    //p' | sort | tr '\n' '|')"
want="1 1|1 2|2 1|"
if [ "$outs" = "$want" ]; then
    echo "  PASS: the explorer finds 1 1, 1 2 and 2 1. \"1 1\" is the lost update: both threads got the same number."
    exit 0
fi
echo "  FAIL: expected exactly the outputs 1 1 / 1 2 / 2 1, got: ${outs:-nothing}"
echo "        Each thread should: read value into temp; write temp + 1 into value; print temp."
exit 1
