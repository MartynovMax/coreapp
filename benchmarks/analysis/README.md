# analysis — Offline Benchmark Analysis Tool

Python package for offline ingestion, validation, reporting, comparison,
and plotting of structured benchmark outputs produced by the C++ benchmark runner.

## Responsibilities

### C++ is responsible for

- Generating structured output files (`.jsonl`, `.csv`).
- Defining and preserving output schemas (`ts.v2`, `summary.v2`).
- Schema versioning and guarantees.
- Correctness and completeness of produced files.

### Python is responsible for

- **Ingestion** — reading `.jsonl` and `.csv` files into typed Python models.
- **Validation** — checking schema versions, required fields, and NA semantics.
- **Reporting** — generating compact tables and summaries from a single run.
- **Comparison** — diffing two runs across key metrics; NA-safe (NA ≠ 0).
- **Plotting** — producing charts and figures for offline review.

## Module structure

| Module                | Responsibility                                      |
|-----------------------|-----------------------------------------------------|
| `models.py`           | Internal domain dataclasses (`RunModel`, etc.)      |
| `ingest_summary.py`   | Load summary.v2 CSV → `SummaryRecord` list          |
| `ingest_timeseries.py`| Load ts.v2 JSONL → `TimeSeriesRecord` list          |
| `validate.py`         | Schema and content validation                       |
| `report.py`           | Single-run report generation                        |
| `compare.py`          | Run-to-run comparison and diff                      |
| `plots.py`            | Chart and figure generation                         |
| `cli.py`              | `argparse` entry point (`report`, `compare`)        |

## Usage

```bash
# Generate a report for a single run
python -m analysis.cli report --input results/run1

# Compare two runs
python -m analysis.cli compare --baseline results/run1 --candidate results/run2
```

## Input file conventions

The C++ runner produces two files per run (with `--out=<base_path>`):

- `<base_path>.jsonl` — time-series event log (schema `ts.v2`)
- `<base_path>.csv`  — aggregated summary metrics (schema `summary.v2`)

Pass `<base_path>` (without extension) as the `--input`, `--baseline`,
or `--candidate` argument.

## Dependencies

At skeleton stage: Python stdlib only (`csv`, `json`, `argparse`).  
Future steps will add `matplotlib` and optionally `pandas`.

