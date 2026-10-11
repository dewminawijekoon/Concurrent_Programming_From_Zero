# Checks the blocking-lock version of the priority story.
# TIMEOUT: 60
I="$ROOT/tools/interleave/interleave.py"
code="$(sed 's/#.*//' "$EX_SRC")"
if [ "$(grep -cE 'm\.down\(\)|down\(m\)' <<<"$code")" -lt 2 ] || [ "$(grep -cE 'm\.up\(\)|up\(m\)' <<<"$code")" -lt 2 ] || [ "$(grep -c '^ *critical' <<<"$code")" -lt 2 ]; then
    echo "  FAIL: the model still has TODOs (each thread needs m.down(), critical, m.up())"; exit 1; fi
grep -q 'atomic' <<<"$code" && { echo "  FAIL: this exercise is about the blocking lock: use m.down()/m.up(), not TestAndSet"; exit 1; }
prio="$(python3 "$I" "$EX_SRC" --no-halt --priority H=2,L=1 --verdict 2>&1)"
echo "$prio" | sed 's/^/  | /' | head -20
v="$(grep '^VERDICT' <<<"$prio")"
for want in "mutex=ok" "deadlock=none" "livelock=none"; do
    grep -q "$want" <<<"$v" || { echo "  FAIL: expected $want with --priority H=2,L=1. Does L set woken = 1 after m.down()?"; exit 1; }
done
echo "  PASS: H sleeps instead of spinning, so L gets the CPU, releases the lock and wakes H."
exit 0
