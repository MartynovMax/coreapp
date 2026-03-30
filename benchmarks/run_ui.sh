#!/usr/bin/env bash
# Launch the benchmark analysis desktop UI.
# Run from any directory:
#   ./benchmarks/run_ui.sh
#
# Requirements:
#   pip install -r benchmarks/analysis/requirements.txt
#
# Tkinter is included with standard Python — no extra install needed.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
python "$SCRIPT_DIR/analysis/ui.py"
