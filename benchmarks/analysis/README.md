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
- **Reporting** — generating compact Markdown tables and summaries from a single run.
- **Comparison** — diffing two runs across key metrics; NA-safe (`NA ≠ 0`).
- **Filtering** — restricting outputs by allocator, workload, profile, or experiment.
- **Plotting** — producing static charts for offline review.

---

## Module structure

| Module                 | Responsibility                                                  |
|------------------------|-----------------------------------------------------------------|
| `models.py`            | Internal domain dataclasses (`RunModel`, `SummaryRecord`, etc.) |
| `ingest_summary.py`    | Load `summary.v2` CSV → `SummaryRecord` list                   |
| `ingest_timeseries.py` | Load `ts.v2` JSONL → `TimeSeriesRecord` list                   |
| `validate.py`          | Schema version and required-field validation                    |
| `assemble.py`          | Combine ingested records into a canonical `RunModel`            |
| `filters.py`           | `RunFilter` dataclass; `apply_filter` / `filter_records`        |
| `report.py`            | Single-run Markdown report generation                           |
| `compare.py`           | Run-to-run comparison, NA-safe diff, Markdown output            |
| `plots.py`             | Static chart generation (latency, throughput, time-series)      |
| `cli.py`               | `argparse` entry point (`report`, `compare`, `plot`)            |
| `tests/`               | pytest test suite                                               |

---

## Input file conventions

The C++ runner produces two files per run (with `--out=<base_path>`):

| File                    | Schema       | Content                               |
|-------------------------|--------------|---------------------------------------|
| `<base_path>.csv`       | `summary.v2` | Aggregated per-repetition metrics     |
| `<base_path>.jsonl`     | `ts.v2`      | Time-series event / tick / warning log|

Pass `<base_path>` (without extension) as `--input`, `--baseline`, or `--candidate`.  
Either file may be absent; a warning is emitted for the missing stream.

---

## CLI usage

### Generate a report for a single run

```bash
python -m analysis.cli report --input results/run1
```

### Compare two runs

```bash
python -m analysis.cli compare \
  --baseline  results/run_v1 \
  --candidate results/run_v2
```

### Generate plots

```bash
# Latency bar chart (min / median / p95 / max per record)
python -m analysis.cli plot \
  --input  results/run1 \
  --kind   latency \
  --output latency.png

# Throughput comparison across two runs
python -m analysis.cli plot \
  --input   results/run1 \
  --compare results/run2 \
  --kind    throughput \
  --output  throughput.png

# Time-series trend from JSONL tick records
python -m analysis.cli plot \
  --input      results/run1 \
  --kind       timeseries \
  --metric-key live_bytes \
  --output     live_bytes.png
```

### Filter options (all commands, all repeatable)

```bash
# Restrict to one allocator
python -m analysis.cli report --input results/run1 --allocator tlsf

# Restrict to multiple allocators
python -m analysis.cli compare \
  --baseline  results/run1 \
  --candidate results/run2 \
  --allocator tlsf --allocator mimalloc

# Restrict to a specific experiment
python -m analysis.cli report --input results/run1 --experiment exp_mixed_sizes

# Combine filters (AND semantics across dimensions)
python -m analysis.cli report --input results/run1 \
  --allocator tlsf \
  --experiment exp_mixed_sizes
```

Available filter flags (work on `report`, `compare`, and `plot`):

| Flag            | Filters on                        |
|-----------------|-----------------------------------|
| `--allocator`   | `SummaryRecord.metadata.allocator`|
| `--experiment`  | `SummaryRecord.metadata.experiment_name` |
| `--workload`    | `SummaryRecord.workload`          |
| `--profile`     | `SummaryRecord.profile`           |

> **Note:** `--workload` and `--profile` apply to summary-based outputs only.
> For `--kind=timeseries` plots only `--allocator` and `--experiment` are used;
> the `timeseries_filter` parameter restricts plotted groups accordingly.

---

## NA semantics

Empty CSV cells and absent JSONL fields are preserved as `None` throughout the
pipeline.  The string `"NA"` is used in all text outputs.

Rules enforced everywhere:
- `NA` is **never coerced to `0`** during ingestion, validation, or comparison.
- In comparison, `NA` on either side produces an explicit `INCOMPARABLE_*` or
  `BOTH_NA` status — never a numeric diff.
- In plots, NA cells are rendered as a hatched placeholder bar, not a zero bar.

---

## Running the tests

```bash
# From the benchmarks/ directory
python -m pytest analysis/tests/ -v
```

The test suite covers:

| Test file                    | What is tested                                          |
|------------------------------|---------------------------------------------------------|
| `test_ingest_summary.py`     | CSV parsing, NA preservation, malformed values, errors  |
| `test_ingest_timeseries.py`  | JSONL parsing, payload edge cases, malformed values     |
| `test_validate.py`           | Schema version checks, required fields, NA allowed      |
| `test_assemble.py`           | RunModel assembly, stream mismatch, file loading        |
| `test_report.py`             | Report structure, NA rendering, determinism, filtering  |
| `test_compare.py`            | Matching, NA handling, direction classification, filters |
| `test_cli.py`                | CLI exit codes, subcommand registration, filter args    |

---

## Dependencies

| Package      | Required | Purpose                          |
|--------------|----------|----------------------------------|
| `matplotlib` | Yes      | Static chart generation          |
| `numpy`      | Yes      | Array ops (pulled in by matplotlib) |
| `pytest`     | Dev only | Test runner                      |

Standard library only for all ingestion, validation, reporting, and comparison
modules (`csv`, `json`, `argparse`, `dataclasses`, `warnings`).
