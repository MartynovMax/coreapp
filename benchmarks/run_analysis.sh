#!/usr/bin/env bash
# Run the analysis CLI tool.
# Usage examples:
#
#   report:
#     ./run_analysis.sh report --input results/run1
#     ./run_analysis.sh report --input results/run1 --allocator tlsf
#
#   compare:
#     ./run_analysis.sh compare --baseline results/run1 --candidate results/run2
#
#   plot:
#     ./run_analysis.sh plot --input results/run1 --kind latency --output latency.png
#     ./run_analysis.sh plot --input results/run1 --kind timeseries \
#         --metric-key live_bytes --output ts.png
#
#   help:
#     ./run_analysis.sh --help
#     ./run_analysis.sh report --help

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"
python -m analysis "$@"

