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

**Schema Version:** `ts.v2`

### Record Types

- `event` — Lifecycle events (ExperimentBegin/End, PhaseBegin/End/Complete)
- `tick` — Periodic progress checkpoints
- `warning` — Failures (OutOfMemory, AllocationFailed, PhaseFailure)

**Note:** `snapshot` record type is reserved for future measurement system snapshots.

### Required Fields

All records contain:

```json
{
  "schema_version": "ts.v2",
  "record_type": "event|tick|warning",
  "run_id": "unique_run_identifier",
  "experiment_name": "experiment_name",
  "experiment_category": "category",
  "allocator": "allocator_name",
  "seed": 42,
  "run_timestamp_utc": "1739790600000000000",
  "os_name": "Windows",
  "os_version": "10.0.19045",
  "arch": "x64",
  "compiler_name": "MSVC",
  "compiler_version": "1939",
  "build_type": "Release",
  "build_flags": "debug=0;opt=1;asserts=0",
  "cpu_model": "Intel(R) Core(TM) i7-9700K CPU @ 3.60GHz",
  "cpu_cores_logical": 8,
  "cpu_cores_physical": 8,
  "repetition_id": 0,
  "timestamp_ns": 123456789,
  "event_seq_no": 10,
  "payload": { /* type-specific data */ }
}
```

### Environment Metadata Fields

| Field | Type | Description | Fallback |
|-------|------|-------------|----------|
| `run_timestamp_utc` | string | Run start timestamp (nanoseconds, from HighResTimer) | `"0"` |
| `os_name` | string | Operating system name (Windows/Linux/macOS) | `"unknown"` |
| `os_version` | string | OS version string | `"unknown"` |
| `arch` | string | CPU architecture (x64/ARM64/etc.) | `"unknown"` |
| `compiler_name` | string | Compiler name (MSVC/GCC/Clang) | `"unknown"` |
| `compiler_version` | string | Compiler version string | `"unknown"` |
| `build_type` | string | Build configuration (Debug/Release) | `"unknown"` |
| `build_flags` | string | Normalized key build flags | `"unknown"` |
| `cpu_model` | string | CPU model string | `"unknown"` |
| `cpu_cores_logical` | u32 | Logical CPU cores | `0` |
| `cpu_cores_physical` | u32 | Physical CPU cores | `0` |

**Note:** Environment metadata is collected once per run and included in every record for offline analysis. Fallback values ensure schema stability when runtime queries fail. The `run_timestamp_utc` field contains the same value as the existing `startTimestampNs` but formatted as a string for consistency with other metadata fields.

### Example: PhaseComplete Event (v2)

```json
{
  "schema_version": "ts.v2",
  "record_type": "event",
  "run_id": "run_1708095234_seed42",
  "experiment_name": "simple_alloc",
  "experiment_category": "allocation",
  "allocator": "DefaultAllocator",
  "seed": 42,
  "run_timestamp_utc": "1739790600000000000",
  "os_name": "Windows",
  "os_version": "10.0.19045",
  "arch": "x64",
  "compiler_name": "MSVC",
  "compiler_version": "1939",
  "build_type": "Release",
  "build_flags": "debug=0;opt=1;asserts=0",
  "cpu_model": "Intel(R) Core(TM) i7-9700K CPU @ 3.60GHz",
  "cpu_cores_logical": 8,
  "cpu_cores_physical": 8,
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

### Example: Tick Event (v2)

```json
{
  "schema_version": "ts.v2",
  "record_type": "tick",
  "run_id": "run_1708095234_seed42",
  "experiment_name": "simple_alloc",
  "allocator": "DefaultAllocator",
  "seed": 42,
  "run_timestamp_utc": "1739790600000000000",
  "os_name": "Windows",
  "os_version": "10.0.19045",
  "arch": "x64",
  "compiler_name": "MSVC",
  "compiler_version": "1939",
  "build_type": "Release",
  "build_flags": "debug=0;opt=1;asserts=0",
  "cpu_model": "Intel(R) Core(TM) i7-9700K CPU @ 3.60GHz",
  "cpu_cores_logical": 8,
  "cpu_cores_physical": 8,
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

**Schema Version:** `summary.v2`

### Required Columns

| Column | Type | Description |
|--------|------|-------------|
| `schema_version` | string | Always `summary.v2` |
| `run_id` | string | Unique run identifier |
| `experiment_name` | string | Experiment name |
| `experiment_category` | string | Experiment category |
| `allocator` | string | Allocator name |
| `seed` | u64 | Deterministic seed |
| `warmup_iterations` | u32 | Warmup count |
| `measured_repetitions` | u32 | Repetition count |

### Environment and Build Metadata Columns

| Column | Type | Description | Fallback |
|--------|------|-------------|----------|
| `run_timestamp_utc` | string | Run start timestamp (nanoseconds, from HighResTimer) | `"0"` |
| `os_name` | string | Operating system name (Windows/Linux/macOS) | `"unknown"` |
| `os_version` | string | OS version string | `"unknown"` |
| `arch` | string | CPU architecture (x64/ARM64/etc.) | `"unknown"` |
| `compiler_name` | string | Compiler name (MSVC/GCC/Clang) | `"unknown"` |
| `compiler_version` | string | Compiler version string | `"unknown"` |
| `build_type` | string | Build configuration (Debug/Release) | `"unknown"` |
| `build_flags` | string | Normalized key build flags | `"unknown"` |
| `cpu_model` | string | CPU model string | `"unknown"` |
| `cpu_cores_logical` | u32 | Logical CPU cores | `0` |
| `cpu_cores_physical` | u32 | Physical CPU cores | `0` |

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
- `ts.v2` — **Current.** Added environment and build metadata (11 new fields: run_timestamp_utc, os_name, os_version, arch, compiler_name, compiler_version, build_type, build_flags, cpu_model, cpu_cores_logical, cpu_cores_physical)
- `summary.v1` — Initial summary schema
- `summary.v2` — **Current.** Added environment and build metadata (11 new columns matching ts.v2 fields)

### Migration Guide (v1 → v2)

**Breaking Change:** Field positions in CSV changed. Parsers relying on column order must be updated.

**Backward Compatibility:**
- v1 parsers can skip unknown fields in JSONL (standard JSON practice)
- v1 CSV parsers must be updated to handle new columns or use header-based parsing

**Recommended Actions:**
- Update parsers to use `schema_version` field for version detection
- Use header-based CSV parsing instead of positional indexing
- For v1 compatibility, filter records by `schema_version` field

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
- **BREAKING:** Schema version bump to `ts.v2` and `summary.v2`
- **Feature:** Added environment and build metadata to all outputs (11 new fields)
  - Runtime: `run_timestamp_utc`, `os_version`, `cpu_model`, `cpu_cores_logical`, `cpu_cores_physical`
  - Compile-time: `os_name`, `arch`, `compiler_name`, `compiler_version`, `build_type`, `build_flags`
- **Reliability:** All metadata fields use safe fallback values (`"unknown"` or `0`) when unavailable
- **Purpose:** Enable offline comparison and interpretation of benchmark results across different environments
- **Security:** Added JSON string escaping for all metadata fields to prevent malformed JSON
- **Validation:** CLI now requires `--out=<path>` when using structured outputs (jsonl/summary)
- **Documentation:** Clarified that `snapshot` record type is reserved for future use
