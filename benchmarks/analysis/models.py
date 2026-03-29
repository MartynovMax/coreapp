"""
models.py — internal domain model for offline analysis.

These dataclasses are preliminary internal skeletons. They represent the
Python-side view of benchmark data AFTER ingestion and are NOT a direct
mirror of the raw CSV/JSONL schemas. Schema mapping is the responsibility
of ingest_summary.py and ingest_timeseries.py.

Fields and types here will evolve as ingestion is implemented.

Schema versions this tool targets:
  - ts.v2      (time-series JSONL)
  - summary.v2 (aggregated CSV)
"""

from __future__ import annotations

from dataclasses import dataclass, field
from typing import Optional


@dataclass
class RunMetadata:
    """Environment and run identity fields shared by all records."""

    schema_version: str = ""
    run_id: str = ""
    experiment_name: str = ""
    experiment_category: str = ""
    allocator: str = ""
    seed: int = 0
    run_timestamp_utc: Optional[str] = None

    # Environment
    os_name: str = "unknown"
    os_version: str = "unknown"
    arch: str = "unknown"
    compiler_name: str = "unknown"
    compiler_version: str = "unknown"
    build_type: str = "unknown"
    build_flags: str = "unknown"
    cpu_model: str = "unknown"
    cpu_cores_logical: int = 0
    cpu_cores_physical: int = 0


@dataclass
class SummaryRecord:
    """One row from a summary.v2 CSV file."""

    metadata: RunMetadata = field(default_factory=RunMetadata)

    warmup_iterations: Optional[int] = None
    measured_repetitions: Optional[int] = None

    # Timer metrics (ns)
    phase_duration_ns: Optional[int] = None
    repetition_min_ns: Optional[int] = None
    repetition_max_ns: Optional[int] = None
    repetition_mean_ns: Optional[float] = None
    repetition_median_ns: Optional[int] = None
    repetition_p95_ns: Optional[int] = None

    # Counter metrics
    alloc_ops_total: Optional[int] = None
    free_ops_total: Optional[int] = None
    bytes_allocated_total: Optional[int] = None
    bytes_freed_total: Optional[int] = None
    peak_live_count: Optional[int] = None
    peak_live_bytes: Optional[int] = None
    final_live_count: Optional[int] = None
    final_live_bytes: Optional[int] = None

    # Derived metrics
    ops_per_sec_mean: Optional[float] = None
    throughput_bytes_per_sec_mean: Optional[float] = None


@dataclass
class TimeSeriesRecord:
    """One line from a ts.v2 JSONL file."""

    metadata: RunMetadata = field(default_factory=RunMetadata)

    record_type: str = ""          # "event" | "tick" | "warning"
    repetition_id: int = 0
    timestamp_ns: int = 0
    event_seq_no: int = 0
    phase_name: str = ""

    # Raw payload — kept as a dict until a later step parses it further.
    payload: dict = field(default_factory=dict)


@dataclass
class RunModel:
    """Aggregated view of a single benchmark run (one .jsonl + one .csv pair)."""

    metadata: RunMetadata = field(default_factory=RunMetadata)
    summary: list[SummaryRecord] = field(default_factory=list)
    timeseries: list[TimeSeriesRecord] = field(default_factory=list)

