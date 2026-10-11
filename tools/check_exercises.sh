#!/usr/bin/env bash
# check_exercises.sh - CI guard for the student starting point.
# Every untouched exercise must COMPILE, and its tests must end cleanly:
# PASS, FAIL or SKIP are fine; a compile error, crash or hard hang is not.
cd "$(dirname "$0")/.." || exit 1
bash tools/runtests.sh --quiet $(ls -d modules/[0-9]* 2>/dev/null) > /dev/null
bad=0
for log in modules/*/.build/*.log; do
    [ -e "$log" ] || continue
    if grep -qE "FAIL: compile error|crashed with exit code|killed after" "$log"; then
        echo "BAD: $log"; sed 's/^/    /' "$log" | tail -15; bad=1
    fi
done
[ $bad -eq 0 ] && echo "all exercises compile and their tests end cleanly"
exit $bad
