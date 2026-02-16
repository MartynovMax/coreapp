# Structured Outputs for Benchmarks

## Overview

The benchmark system supports **structured outputs** in addition to human-readable text:

1. **Time-series output (JSONL)** — Event stream for detailed analysis
2. **Summary output (CSV)** — Aggregated metrics for comparison

Both formats use **versioned schemas** with stable metadata fields.

## Usage

### CLI Flags

```bash
# Time-series only (JSONL)
./coreapp_benchmarks --format=jsonl --out=results/run1

# Summary only (CSV)
./coreapp_benchmarks --format=summary --out=results/run1

# All outputs (text + JSONL + CSV)
./coreapp_benchmarks --format=all --out=results/run1

# No outputs (silent)
./coreapp_benchmarks --format=none
```

### Output Files

- `<path>.jsonl` — Time-series event log (JSONL)
- `<path>.csv` — Summary metrics (CSV)

## Time-Series Schema (JSONL)

**Schema Version:** `ts.v1`

### Record Types

- `event` — Lifecycle events (ExperimentBegin/End, PhaseBegin/End/Complete)
- `tick` — Periodic progress checkpoints
- `warning` — Failures (OutOfMemory, AllocationFailed, PhaseFailure)

**Note:** `snapshot` record type is reserved for future measurement system snapshots.

### Required Fields

All records contain:

```json
{
  "schema_version": "ts.v1",
  "record_type": "event|tick|warning",
  "run_id": "unique_run_identifier",
  "experiment_name": "experiment_name",
  "experiment_category": "category",
  "allocator": "allocator_name",
  "seed": 42,
  "repetition_id": 0,
  "timestamp_ns": 123456789,
  "event_seq_no": 10,
  "payload": { /* type-specific data */ }
}
```

### Example: PhaseComplete Event

```json
{
  "schema_version": "ts.v1",
  "record_type": "event",
  "run_id": "run_1708095234_seed42",
  "experiment_name": "simple_alloc",
  "experiment_category": "allocation",
  "allocator": "DefaultAllocator",
  "seed": 42,
  "repetition_id": 1,
  "timestamp_ns": 123456789000,
  "event_seq_no": 42,
  "event_experiment_name": "simple_alloc",
  "phase_name": "Steady",
  "payload": {
    "event_type": "PhaseComplete",
    "duration_ns": 5000000,
    "alloc_count": 1000,
    "free_count": 800,
    "bytes_allocated": 102400,
    "bytes_freed": 81920,
    "ops_per_sec": 200000.0,
    "throughput": 20480000.0
  }
}
```

### Example: Tick Event

```json
{
  "schema_version": "ts.v1",
  "record_type": "tick",
  "run_id": "run_1708095234_seed42",
  "experiment_name": "simple_alloc",
  "allocator": "DefaultAllocator",
  "seed": 42,
  "repetition_id": 1,
  "timestamp_ns": 123450000000,
  "event_seq_no": 23,
  "phase_name": "Steady",
  "payload": {
    "op_index": 500,
    "alloc_count": 500,
    "free_count": 300,
    "bytes_allocated": 51200,
    "bytes_freed": 30720,
    "peak_live_count": 200,
    "peak_live_bytes": 20480
  }
}
```

## Summary Schema (CSV)

**Schema Version:** `summary.v1`

### Required Columns

| Column | Type | Description |
|--------|------|-------------|
| `schema_version` | string | Always `summary.v1` |
| `run_id` | string | Unique run identifier |
| `experiment_name` | string | Experiment name |
| `experiment_category` | string | Experiment category |
| `allocator` | string | Allocator name |
| `seed` | u64 | Deterministic seed |
| `warmup_iterations` | u32 | Warmup count |
| `measured_repetitions` | u32 | Repetition count |

### Timer Metrics

| Column | Type | Unit | Description |
|--------|------|------|-------------|
| `phase_duration_ns` | u64 | ns | Last phase duration |
| `repetition_min_ns` | u64 | ns | Min repetition time |
| `repetition_max_ns` | u64 | ns | Max repetition time |
| `repetition_mean_ns` | f64 | ns | Mean repetition time |
| `repetition_median_ns` | u64 | ns | Median repetition time |
| `repetition_p95_ns` | u64 | ns | 95th percentile time |

### Counter Metrics

| Column | Type | Unit | Description |
|--------|------|------|-------------|
| `alloc_ops_total` | u64 | count | Total allocations |
| `free_ops_total` | u64 | count | Total deallocations |
| `bytes_allocated_total` | u64 | bytes | Total allocated bytes |
| `bytes_freed_total` | u64 | bytes | Total freed bytes |
| `peak_live_count` | u64 | count | Peak live objects (optional) |
| `peak_live_bytes` | u64 | bytes | Peak live bytes (optional) |
| `final_live_count` | u64 | count | Final live objects |
| `final_live_bytes` | u64 | bytes | Final live bytes |

### Derived Metrics

| Column | Type | Unit | Description |
|--------|------|------|-------------|
| `ops_per_sec_mean` | f64 | ops/sec | Mean operations per second |
| `throughput_bytes_per_sec_mean` | f64 | bytes/sec | Mean throughput |

### NA Handling

- Missing/unavailable metrics → empty CSV cell
- Optional metrics (e.g., peak_live_count) may be NA if capability not available

## Schema Versioning

### Rules

- Format: `<stream>.<major>` (e.g., `ts.v1`, `summary.v1`)
- **Breaking changes** (rename/remove/type change) → increment major
- **Additive changes** (new optional fields) → keep major

### Version History

- `ts.v1` — Initial time-series schema
- `summary.v1` — Initial summary schema

## Integration Points

### Programmatic API

```cpp
#include "benchmarks/output/output_manager.hpp"
#include "benchmarks/output/output_config.hpp"

OutputConfig config;
config.enableTimeSeriesOutput = true;
config.enableSummaryOutput = true;
config.outputPath = "results/experiment1";

OutputManager manager(config);

RunMetadata metadata;
metadata.runId = "run_12345";
metadata.experimentName = "test";
metadata.seed = 42;
manager.Initialize(metadata);

// Feed events
Event evt;
manager.OnEvent(evt);

// Finalize with metrics
MetricCollector collector;
manager.Finalize(collector);
```

## Testing

Unit tests: `tests/test_output.cpp`
Integration tests: `tests/test_output_modes.cpp`

```bash
# Run output tests
./benchmark_tests --gtest_filter=OutputTest.*
./benchmark_tests --gtest_filter=OutputModesTest.*
```

## Future Enhancements

- [ ] JSON summary format (alternative to CSV)
- [ ] Snapshot record type for measurement system data
- [ ] Derived metrics aggregation (ops_per_sec_mean, throughput_mean)
- [ ] Compression support (.jsonl.gz)
- [ ] Streaming to remote storage (S3, etc.)
- [ ] Schema validation tools
- [ ] Offline analysis scripts/notebooks

## Recent Changes

### 2026-02-16
- **Security:** Added JSON string escaping for all metadata fields to prevent malformed JSON
- **Validation:** CLI now requires `--out=<path>` when using structured outputs (jsonl/summary)
- **Documentation:** Clarified that `snapshot` record type is reserved for future use
