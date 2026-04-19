#!/usr/bin/env bash
# Batch run: execute full article1 experiment matrix with auto-generated output.
#
# Usage:
#   ./run_batch.sh                              (default: article1/*, 5 reps)
#   ./run_batch.sh --repetitions=10             (override repetitions)
#   ./run_batch.sh --filter=article1/malloc/*   (subset of matrix)
#
# Output goes to: runs/article1/<YYYYMMDD_HHMMSS>/

set -e

WORK_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$WORK_DIR"

echo "[batch] Starting full matrix batch run..."
echo "[batch] Working dir: $PWD"
echo

./run_benchmarks.sh --batch --config=config/article1_matrix.json --filter=article1/* --format=all --measurements=timer,counter "$@"


