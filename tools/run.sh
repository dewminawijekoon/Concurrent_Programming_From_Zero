#!/usr/bin/env bash
# run.sh — compile one exercise on its own (with its main()) and run it.
#
# Usage: tools/run.sh [--answers] <module-dir> <exercise-name> [args...]
#   e.g. tools/run.sh modules/03-pthreads ex2_global_sum 8
#
# Flags come from a "// BUILD:" comment in the exercise, like the tests.
# MPI exercises ("// REQUIRES: mpi") are built with mpicc and started with
# mpiexec; set NP to choose the number of processes (default 4).
set -u
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
CC="${CC:-gcc}"
answers=0
[ "${1:-}" = "--answers" ] && { answers=1; shift; }
[ $# -ge 2 ] || { echo "usage: $0 [--answers] <module-dir> <exercise-name> [args...]"; exit 2; }
mod="$(cd "$1" && pwd)"; ex="$2"; shift 2
src_root="$mod"; [ $answers -eq 1 ] && src_root="$ROOT/answers/$(basename "$mod")"

# exercises first, then demos (break_it/, calc/, sim/) anywhere in the module
src="$(find "$src_root/exercises" \( -name "$ex.c" -o -name "$ex.cu" \) 2>/dev/null | head -1)"
[ -n "$src" ] || src="$(find "$src_root" -path '*/.build*' -prune -o \( -name "$ex.c" -o -name "$ex.cu" \) -print 2>/dev/null | head -1)"
[ -n "$src" ] || { echo "no exercise or demo named $ex in $src_root"; exit 2; }

magic() { sed -n '1,30p' "$1" | sed -n "s|^[[:space:]]*//[[:space:]]*$2:[[:space:]]*||p" | head -1; }
extra="$(magic "$src" BUILD)"; reqs="$(magic "$src" REQUIRES)"
out="$mod/.build/run_$ex"; mkdir -p "$mod/.build"
flags="-std=gnu17 -O2 -g -Wall -Wextra -Wno-unused-parameter -pthread -I$ROOT/common -I$(dirname "$src")"

if [[ "$src" == *.cu ]]; then
    nvcc -O2 -I"$ROOT/common" -o "$out" "$src" $extra || exit 1
    exec "$out" "$@"
elif [[ " $reqs " == *" mpi "* ]]; then
    mpicc $flags -o "$out" "$src" $extra -lm || exit 1
    MPIEXEC="mpiexec --oversubscribe"; [ "$(id -u)" = "0" ] && MPIEXEC="$MPIEXEC --allow-run-as-root"
    exec $MPIEXEC -n "${NP:-4}" "$out" "$@"
else
    $CC $flags -o "$out" "$src" $extra -lm || exit 1
    exec "$out" "$@"
fi
