# Checks the student's Bill and Ben model: no run gets stuck, and every
# possible output follows the rule.
# TIMEOUT: 60
IL="$ROOT/tools/interleave/interleave.py"
code="$(sed 's/#.*//' "$EX_SRC")"     # ignore comments
if ! grep -qE 'down|wait|P\(' <<<"$code" || ! grep -qE 'up|signal|V\(' <<<"$code"; then
    echo "  FAIL: the model has no semaphore operations yet (TODO 1 and TODO 2)"; exit 1
fi
out="$(python3 "$IL" "$EX_SRC" --outputs --show 1000 2>&1)"
echo "$out" | sed 's/^/  | /' | head -15
if grep -q "STUCK" <<<"$out"; then
    echo "  FAIL: some runs get stuck (deadlock). If BOTH threads wait (down) before they signal (up), each waits for the other forever."; exit 1
fi
python3 - "$out" <<'PY'
import sys
lines = [l.strip() for l in sys.argv[1].splitlines() if l.startswith("    ")]
outs = [l.split() for l in lines if set(l.split()) <= set("sDpE") and l]
if not outs:
    print("  FAIL: no complete output found - does the program finish?"); sys.exit(1)
for o in outs:
    s = p = d = e = 0
    for t in o:
        if t == "s": s += 1
        elif t == "p": p += 1
        elif t == "D":
            d += 1
            if p < d:
                print(f"  FAIL: output '{' '.join(o)}' is possible: Bill delivers round {d} before Ben made that soup"); sys.exit(1)
        elif t == "E":
            e += 1
            if s < e:
                print(f"  FAIL: output '{' '.join(o)}' is possible: Ben delivers round {e} before Bill made those sandwiches"); sys.exit(1)
    if len(o) != 8:
        print(f"  FAIL: output '{' '.join(o)}' does not have 8 prints"); sys.exit(1)
print(f"  PASS: all {len(outs)} possible outputs follow the rule, and no run gets stuck.")
PY
