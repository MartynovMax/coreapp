#!/usr/bin/env bash
# Run the benchmark executable from the workspace/ directory.
# All relative paths (config/, runs/) resolve from here.
#
# Usage examples:
#
#   Basic run (text output):
#     ./run_benchmarks.sh
#
#   With structured output:
#     ./run_benchmarks.sh --format=all --out=runs/manual/run1
#
#   Filter specific experiments:
#     ./run_benchmarks.sh --filter=article1/malloc/*
#
#   Full dataset run (all outputs + manifest):
#     ./run_benchmarks.sh --format=all --out=runs/run1 --repetitions=10
#
# Override executable: BENCH_EXE=<path> ./run_benchmarks.sh

set -e

WORK_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$WORK_DIR"

# Locate the executable
if [ -z "$BENCH_EXE" ]; then
    for P in \
        "../build/bench/benchmarks/coreapp_benchmarks" \
        "../build/benchmarks/coreapp_benchmarks" \
        "../cmake-build-debug/benchmarks/coreapp_benchmarks" \
        "../cmake-build-release/benchmarks/coreapp_benchmarks"
    do
        if [ -x "$P" ]; then
            BENCH_EXE="$P"
            break
        fi
    done
fi

if [ -z "$BENCH_EXE" ]; then
    echo "ERROR: Could not find coreapp_benchmarks"
    echo "Build first, or set BENCH_EXE=<path>"
    exit 1
fi

echo "[run_benchmarks] Working dir: $PWD"
echo "[run_benchmarks] Executable:  $BENCH_EXE"
echo

exec "$BENCH_EXE" "$@"


