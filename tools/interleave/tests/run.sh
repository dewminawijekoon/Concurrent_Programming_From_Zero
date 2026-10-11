#!/usr/bin/env bash
# Regression tests for interleave.py: verdicts must match the lecture notes.
cd "$(dirname "$0")/.." || exit 1
fail=0
check() { # check "<description>" "<expected substring>" <args...>
    local desc="$1" want="$2"; shift 2
    local got; got="$(python3 interleave.py "$@" 2>&1)"
    if grep -qF -- "$want" <<<"$got"; then echo "  ok    $desc"
    else echo "  FAIL  $desc"; echo "        expected: $want"; echo "$got" | sed 's/^/        | /' | tail -5; fail=1; fi
}
V="--verdict"
echo "== lecture verdicts (L05, L06)"
check "Attempt 1 blocks without contention"  "blocked=found deadlock=none livelock=none mutex=ok"  programs/attempt1.il $V
check "Attempt 2 violates mutual exclusion"  "mutex=violated"                                      programs/attempt2.il $V
check "Attempt 3 deadlocks"                  "deadlock=found livelock=none mutex=ok"               programs/attempt3.il $V
check "Attempt 4 livelocks"                  "livelock=found mutex=ok"                             programs/attempt4.il $V
check "Attempt 4 can starve"                 "starvation=found"                                    programs/attempt4.il $V
check "Dekker is correct"                    "blocked=none deadlock=none livelock=none mutex=ok starvation=none" programs/dekker.il $V
check "Peterson is correct"                  "blocked=none deadlock=none livelock=none mutex=ok starvation=none" programs/peterson.il $V
check "LockOne deadlocks (concurrent)"       "deadlock=found"                                      programs/lockone.il $V
check "LockTwo blocks (sequential)"          "blocked=found"                                       programs/locktwo.il $V
check "Filter (3 threads) is correct"        "blocked=none deadlock=none livelock=none mutex=ok starvation=none" programs/filter3.il $V
check "Bakery (2 threads) is correct"        "blocked=none deadlock=none livelock=none mutex=ok starvation=none" programs/bakery2.il $V --bound 4
echo "== hardware locks"
check "TAS lock is safe but can starve"      "mutex=ok starvation=found"                           programs/tas_lock.il $V
check "non-atomic TAS breaks mutex"          "mutex=violated"                                      programs/tas_nonatomic.il $V
check "TAS + strict priority livelocks"      "LIVELOCK"                                            programs/tas_priority.il --priority A=2,B=2,C=1 --no-halt
check "TAS without priority is fine"         "livelock=none mutex=ok"                              programs/tas_priority.il --no-halt $V
echo "== outputs"
check "counter race can lose an update"      "1 1"                                                 programs/counter_race.il --outputs
check "puzzle: B B A possible"               "IS possible"                                         programs/puzzle_ab.il --why "B B A"
check "puzzle: A first impossible"           "IMPOSSIBLE"                                          programs/puzzle_ab.il --why "A B"
check "puzzle: (B A) forever possible"       "IS possible"                                         programs/puzzle_ab.il --infinite "B A"
check "puzzle: (A B) forever impossible"     "IMPOSSIBLE"                                          programs/puzzle_ab.il --infinite "A B"
check "lightswitch: A E F B C D possible"    "IS possible"                                         programs/lightswitch.il --why "A E F B C D"
check "lightswitch: writer inside readers"   "IMPOSSIBLE"                                          programs/lightswitch.il --why "A C D B E F"
[ $fail -eq 0 ] && echo "all interleave tests passed"
exit $fail
