#!/usr/bin/env bash
# Launch the benchmark analysis desktop UI.
#
# Requirements:
#   pip install -r ../benchmarks/analysis/requirements.txt

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
export PYTHONPATH="$SCRIPT_DIR/../benchmarks"
python "$SCRIPT_DIR/../benchmarks/analysis/ui.py"


