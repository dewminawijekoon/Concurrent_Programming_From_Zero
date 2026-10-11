#!/usr/bin/env bash
# runtests.sh — build and run the self-tests of one or more modules.
#
# Usage: tools/runtests.sh [options] <module-dir>...
#   --answers        test the reference solutions in answers/ instead of yours
#   --tsan           build with ThreadSanitizer (finds data races)
#   --only NAME      run only tests/test_NAME.*
#   --quiet          one line per test
#   --summary FILE   append "module passed total" lines to FILE (used by `make progress`)
#
# How a test is found and built
#   tests/test_<name>.c  is compiled together with  exercises/*/<name>.c
#   (or answers/<module>/exercises/*/<name>.c with --answers), with -DTESTING
#   so the exercise's own main() is left out.
#   tests/test_<name>.sh is run with these environment variables set:
#     EX_SRC (path to <name>.c), CC, CFLAGS, BUILD_DIR, ROOT, MPIEXEC
#
# Magic comments inside a test file (first 30 lines):
#   // BUILD: -fopenmp -lm        extra compiler/linker flags
#   // REQUIRES: openmp mpi cuda avx2   skip (exit 77) if missing
#   // TIMEOUT: 30                hard limit in seconds (default 60)
#
# Exit codes of a test: 0 PASS, 1 FAIL, 77 SKIP. Anything else is a crash.

set -u

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
CC="${CC:-gcc}"
BASE_CFLAGS="-std=gnu17 -O2 -g -Wall -Wextra -Wno-unused-parameter -pthread -I$ROOT/common"

answers=0; tsan=0; only=""; quiet=0; summary=""
modules=()
while [ $# -gt 0 ]; do
    case "$1" in
        --answers) answers=1 ;;
        --tsan) tsan=1 ;;
        --only) only="$2"; shift ;;
        --quiet) quiet=1 ;;
        --summary) summary="$2"; shift ;;
        -h|--help) sed -n '2,25p' "$0"; exit 0 ;;
        *) modules+=("$1") ;;
    esac
    shift
done
[ ${#modules[@]} -gt 0 ] || { echo "usage: $0 [--answers] [--tsan] [--only NAME] <module-dir>..."; exit 2; }

if [ -t 1 ]; then G=$'\e[32m'; R=$'\e[31m'; Y=$'\e[33m'; B=$'\e[1m'; N=$'\e[0m'; else G=; R=; Y=; B=; N=; fi

# ---------------------------------------------------------------- toolchain probes
have_openmp() { echo 'int main(void){return 0;}' | $CC -fopenmp -x c - -o /dev/null 2>/dev/null; }
have_mpi()    { command -v mpicc >/dev/null 2>&1 && command -v mpiexec >/dev/null 2>&1; }
have_cuda()   { command -v nvcc >/dev/null 2>&1 && nvidia-smi >/dev/null 2>&1; }
have_avx2()   { grep -qw avx2 /proc/cpuinfo 2>/dev/null; }
# TSan on newer kernels needs ASLR off for the test process ("unexpected memory
# mapping"); `setarch -R` does that for one process only.
TSAN_RUN=""
have_tsan()   {
    local p=/tmp/.tsan_probe.$$
    echo 'int main(void){return 0;}' | $CC -fsanitize=thread -x c - -o $p 2>/dev/null || return 1
    # Prefer setarch whenever it works: whether TSan crashes without it depends on
    # the random memory layout, so a single plain probe can pass by luck.
    if command -v setarch >/dev/null && setarch "$(uname -m)" -R $p >/dev/null 2>&1; then
        TSAN_RUN="setarch $(uname -m) -R"; rm -f $p; return 0; fi
    if $p >/dev/null 2>&1; then rm -f $p; return 0; fi
    rm -f $p; return 1
}

MPIEXEC="mpiexec --oversubscribe"
if [ "$(id -u)" = "0" ]; then MPIEXEC="$MPIEXEC --allow-run-as-root"; fi

magic() { # magic <file> <KEY>
    sed -n '1,30p' "$1" | sed -n -E "s@^[[:space:]]*(//|#)[[:space:]]*$2:[[:space:]]*@@p" | head -1
}

missing_requirement() { # prints the first unmet requirement, if any
    for req in $1; do
        case "$req" in
            openmp) have_openmp || { echo "OpenMP (gcc -fopenmp)"; return; } ;;
            mpi)    have_mpi    || { echo "MPI (mpicc/mpiexec)"; return; } ;;
            cuda)   have_cuda   || { echo "CUDA (nvcc + NVIDIA GPU) - see module 12 README for the Colab route"; return; } ;;
            avx2)   have_avx2   || { echo "a CPU with AVX2"; return; } ;;
        esac
    done
}

if [ $tsan -eq 1 ] && ! have_tsan; then
    echo "${Y}ThreadSanitizer is not available with $CC on this machine.${N}"
    echo "Use the dev container (see README) or install a recent gcc/clang."
    exit 77
fi

total_pass=0; total_fail=0; total_skip=0

for mod in "${modules[@]}"; do
    mod="$(cd "$mod" && pwd)"
    name="$(basename "$mod")"
    if [ $answers -eq 1 ]; then src_root="$ROOT/answers/$name"; label="answers"; else src_root="$mod"; label="your code"; fi
    build="$mod/.build"; [ $answers -eq 1 ] && build="$mod/.build-answers"; [ $tsan -eq 1 ] && build="$build-tsan"
    mkdir -p "$build"

    shopt -s nullglob
    tests=("$mod"/tests/test_*.c "$mod"/tests/test_*.sh)
    shopt -u nullglob
    [ ${#tests[@]} -gt 0 ] || continue

    echo "${B}== $name ($label)${N}"
    mp=0; mf=0; ms=0
    for t in "${tests[@]}"; do
        base="$(basename "$t")"; tname="${base#test_}"; tname="${tname%.*}"
        [ -n "$only" ] && [ "$tname" != "$only" ] && continue

        ex_src="$(find "$src_root/exercises" \( -name "$tname.c" -o -name "$tname.cu" -o -name "$tname.il" \) 2>/dev/null | head -1)"
        extra="$(magic "$t" BUILD)"
        reqs="$(magic "$t" REQUIRES)"
        tmo="$(magic "$t" TIMEOUT)"; tmo="${tmo:-60}"
        cflags="$BASE_CFLAGS"
        [ $tsan -eq 1 ] && cflags="$cflags -fsanitize=thread -O1"

        [ $quiet -eq 0 ] && echo "${B}-- $tname${N}"
        out_file="$build/$tname.log"
        miss="$(missing_requirement "$reqs")"
        if [ -n "$miss" ]; then
            status=77; echo "  SKIP: needs $miss" > "$out_file"
        elif [ -z "$ex_src" ] && [[ "$t" == *.c ]]; then
            status=1; echo "  FAIL: cannot find exercises/*/$tname.c in $src_root" > "$out_file"
        elif [[ "$t" == *.c ]]; then
            ex_dir="$(dirname "$ex_src")"
            if ! $CC $cflags -DTESTING -I"$ex_dir" -o "$build/test_$tname" "$t" "$ex_src" $extra -lm > "$out_file" 2>&1; then
                status=1; { echo "  FAIL: compile error"; } >> "$out_file"
            else
                runner=""; [ $tsan -eq 1 ] && runner="$TSAN_RUN"
                timeout --foreground -k 5 "$tmo" $runner "$build/test_$tname" > "$out_file.run" 2>&1; status=$?
                # keep compiler warnings visible above the run output
                cat "$out_file.run" >> "$out_file"; rm -f "$out_file.run"
                [ $status -eq 124 ] && echo "  FAIL: killed after ${tmo}s (hang: deadlock? livelock? unfinished TODO?)" >> "$out_file"
            fi
        else
            EX_SRC="$ex_src" CC="$CC" CFLAGS="$cflags $extra" BUILD_DIR="$build" ROOT="$ROOT" MPIEXEC="$MPIEXEC" \
                timeout --foreground -k 5 "$tmo" bash "$t" > "$out_file" 2>&1; status=$?
            [ $status -eq 124 ] && echo "  FAIL: killed after ${tmo}s (hang)" >> "$out_file"
        fi

        case $status in
            0)  mp=$((mp+1)); res="${G}PASS${N}" ;;
            77) ms=$((ms+1)); res="${Y}SKIP${N}" ;;
            1|124) mf=$((mf+1)); res="${R}FAIL${N}" ;;
            *)  mf=$((mf+1)); res="${R}CRASH (exit $status)${N}"; echo "  crashed with exit code $status (segfault = 139, abort = 134)" >> "$out_file" ;;
        esac
        if [ $quiet -eq 1 ]; then
            printf "  %-40s %s\n" "$tname" "$res"
        else
            cat "$out_file"; echo "  => $res"
        fi
    done
    echo "${B}   $name: ${G}$mp passed${N}, ${R}$mf failed${N}, ${Y}$ms skipped${N}"
    [ -n "$summary" ] && echo "$name $mp $((mp+mf)) $ms" >> "$summary"
    total_pass=$((total_pass+mp)); total_fail=$((total_fail+mf)); total_skip=$((total_skip+ms))
done

[ ${#modules[@]} -gt 1 ] && echo "${B}TOTAL: ${G}$total_pass passed${N}, ${R}$total_fail failed${N}, ${Y}$total_skip skipped${N}"
[ $total_fail -eq 0 ]
