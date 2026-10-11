# Checks the ordered-forks dining philosophers model.
# TIMEOUT: 60
code="$(sed 's/#.*//' "$EX_SRC")"
if [ "$(grep -cE '\.down\(\)' <<<"$code")" -lt 6 ] || [ "$(grep -c '^ *critical' <<<"$code")" -lt 3 ]; then
    echo "  FAIL: the model still has TODOs (each philosopher needs two down(), critical and two up())"; exit 1; fi
p2="$(sed -n '/thread P2/,$p' <<<"$code")"
first="$(grep -oE 'f[0-2]\.down' <<<"$p2" | head -1)"
[ "$first" = "f0.down" ] || { echo "  FAIL: P2 needs forks 2 and 0; with ordering it must pick up f0 first"; exit 1; }
out="$(python3 "$ROOT/tools/interleave/interleave.py" "$EX_SRC" --verdict 2>&1)"
echo "$out" | sed 's/^/  | /' | head -20
grep -q 'deadlock=none' <<<"$out" || { echo "  FAIL: the explorer still finds a deadlock: does every philosopher take its lower-numbered fork first?"; exit 1; }
echo "  PASS: with the forks taken in number order, no cycle of waiting can form."
exit 0
