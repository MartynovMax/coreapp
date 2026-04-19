# Workspace — Benchmark Working Directory

This directory is the **runtime working directory** for benchmark execution and analysis.
It separates runtime artifacts (configs, scripts, results) from source code (`benchmarks/`).

## Directory Structure

```
workspace/
├── config/                      # Input configuration files
│   └── article1_matrix.json     # Experiment matrix definition
│
├── runs/                        # Output results (gitignored)
│   └── article1/
│       └── 20260419_143000/     # Timestamped batch run
│           ├── manifest.json    # Batch-level manifest (scenarios, statuses)
│           ├── summary.csv      # Aggregated metrics for all scenarios
│           └── raw/             # Per-scenario outputs
│               ├── malloc__fifo__fixed_small/
│               │   ├── data_manifest.json
│               │   ├── data.csv
│               │   └── data.jsonl
│               └── ...
│
├── run_batch.bat / .sh          # One-command full matrix execution
├── run_benchmarks.bat / .sh     # Single/filtered experiment execution
├── run_analysis.bat / .sh       # Python analysis CLI
└── run_ui.bat / .sh             # Analysis desktop UI
```

## Quick Start

### Run the full experiment matrix (Task 9)

```bash
# Windows
run_batch.bat

# Linux/macOS
./run_batch.sh
```

This will:
1. Run all 31 article1 scenarios sequentially
2. Create `runs/article1/<timestamp>/` with auto-generated output
3. Write per-scenario data in `raw/<scenario>/`
4. Merge all summaries into a single `summary.csv`
5. Write a batch `manifest.json` with statuses

### Run a single experiment

```bash
run_benchmarks.bat --filter=article1/malloc/fifo/* --format=all --out=runs/manual/test1
```

### Analyze results

```bash
run_analysis.bat report --input runs/article1/20260419_143000
```

## Configuration

Edit `config/article1_matrix.json` to modify scenarios, workload profiles, or exclusions.
The JSON file in this directory is the canonical runtime config. A copy also exists in
`benchmarks/config/` for unit tests.

