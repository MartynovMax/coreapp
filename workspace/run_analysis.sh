#!/usr/bin/env bash
# Run the analysis CLI tool from the workspace/ directory.
# Usage examples:
#
#   report:
#     ./run_analysis.sh report --input runs/article1/20260419_143000
#
#   compare:
#     ./run_analysis.sh compare --baseline runs/run1 --candidate runs/run2
#
#   help:
#     ./run_analysis.sh --help

set -euo pipefail

WORK_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$WORK_DIR"

export PYTHONPATH="$WORK_DIR/../benchmarks"
python -m analysis "$@"


