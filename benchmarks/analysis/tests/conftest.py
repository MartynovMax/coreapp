"""
conftest.py — shared pytest fixtures for analysis tests.

Provides:
  - Tiny in-memory builders for RunMetadata / SummaryRecord / TimeSeriesRecord
  - tmp_csv / tmp_jsonl helpers that write minimal valid files to tmp_path
  - Pre-built RunModel instances for common test scenarios
"""
from __future__ import annotations

import csv
import io
import json
import os
import textwrap
from typing import Optional

import pytest

from analysis.models import (
    RunMetadata,
    RunModel,
    SummaryRecord,
    TimeSeriesRecord,
)


# ---------------------------------------------------------------------------
# Low-level builders
# ---------------------------------------------------------------------------

def make_metadata(
    *,
    schema_version: str = "summary.v2",
    run_id: str = "run-test-001",
    experiment_name: str = "exp_basic",
    experiment_category: str = "allocation",
    allocator: str = "tlsf",
    seed: Optional[int] = 42,
    run_timestamp_utc: Optional[str] = "2025-01-01T00:00:00Z",
    os_name: str = "Linux",
    os_version: str = "5.15",
    arch: str = "x86_64",
    compiler_name: str = "clang",
    compiler_version: str = "17.0",
    build_type: str = "Release",
    build_flags: str = "-O2",
    cpu_model: str = "Intel Xeon",
    cpu_cores_logical: Optional[int] = 8,
    cpu_cores_physical: Optional[int] = 4,
) -> RunMetadata:
    return RunMetadata(
        schema_version=schema_version,
        run_id=run_id,
        experiment_name=experiment_name,
        experiment_category=experiment_category,
        allocator=allocator,
        seed=seed,
        run_timestamp_utc=run_timestamp_utc,
        os_name=os_name,
        os_version=os_version,
        arch=arch,
        compiler_name=compiler_name,
        compiler_version=compiler_version,
        build_type=build_type,
        build_flags=build_flags,
        cpu_model=cpu_model,
        cpu_cores_logical=cpu_cores_logical,
        cpu_cores_physical=cpu_cores_physical,
    )


def make_summary_record(
    *,
    meta: Optional[RunMetadata] = None,
    status: str = "Valid",
    failure_class: str = "None",
    warmup_iterations: Optional[int] = 3,
    measured_repetitions: Optional[int] = 10,
    phase_duration_ns: Optional[int] = 5_000_000,
    repetition_min_ns: Optional[int] = 400_000,
    repetition_median_ns: Optional[int] = 500_000,
    repetition_mean_ns: Optional[float] = 510_000.0,
    repetition_p95_ns: Optional[int] = 650_000,
    repetition_max_ns: Optional[int] = 900_000,
    alloc_ops_total: Optional[int] = 1_000,
    free_ops_total: Optional[int] = 1_000,
    bytes_allocated_total: Optional[int] = 1_048_576,
    bytes_freed_total: Optional[int] = 1_048_576,
    peak_live_count: Optional[int] = 500,
    peak_live_bytes: Optional[int] = 524_288,
    final_live_count: Optional[int] = 0,
    final_live_bytes: Optional[int] = 0,
    ops_per_sec_mean: Optional[float] = 1_960_784.0,
    throughput_bytes_per_sec_mean: Optional[float] = 2_057_142_857.0,
) -> SummaryRecord:
    return SummaryRecord(
        metadata=meta or make_metadata(),
        status=status,
        failure_class=failure_class,
        warmup_iterations=warmup_iterations,
        measured_repetitions=measured_repetitions,
        phase_duration_ns=phase_duration_ns,
        repetition_min_ns=repetition_min_ns,
        repetition_median_ns=repetition_median_ns,
        repetition_mean_ns=repetition_mean_ns,
        repetition_p95_ns=repetition_p95_ns,
        repetition_max_ns=repetition_max_ns,
        alloc_ops_total=alloc_ops_total,
        free_ops_total=free_ops_total,
        bytes_allocated_total=bytes_allocated_total,
        bytes_freed_total=bytes_freed_total,
        peak_live_count=peak_live_count,
        peak_live_bytes=peak_live_bytes,
        final_live_count=final_live_count,
        final_live_bytes=final_live_bytes,
        ops_per_sec_mean=ops_per_sec_mean,
        throughput_bytes_per_sec_mean=throughput_bytes_per_sec_mean,
    )


def make_ts_record(
    *,
    meta: Optional[RunMetadata] = None,
    record_type: str = "tick",
    repetition_id: int = 0,
    timestamp_ns: int = 1_000_000_000,
    event_seq_no: int = 0,
    phase_name: str = "measure",
    payload: Optional[dict] = None,
    status: str = "Valid",
    failure_class: str = "None",
) -> TimeSeriesRecord:
    ts_meta = meta or make_metadata(schema_version="ts.v2")
    return TimeSeriesRecord(
        metadata=ts_meta,
        record_type=record_type,
        repetition_id=repetition_id,
        timestamp_ns=timestamp_ns,
        event_seq_no=event_seq_no,
        phase_name=phase_name,
        payload=payload if payload is not None else {"live_bytes": 1024},
        status=status,
        failure_class=failure_class,
    )


# ---------------------------------------------------------------------------
# CSV / JSONL file writers
# ---------------------------------------------------------------------------

# Column order that matches what the C++ runner produces for summary.v2
_SUMMARY_COLUMNS = [
    "schema_version", "run_id", "experiment_name", "experiment_category",
    "allocator", "seed", "run_timestamp_utc",
    "os_name", "os_version", "arch",
    "compiler_name", "compiler_version", "build_type", "build_flags",
    "cpu_model", "cpu_cores_logical", "cpu_cores_physical",
    "status", "failure_class",
    "warmup_iterations", "measured_repetitions",
    "phase_duration_ns",
    "repetition_min_ns", "repetition_max_ns", "repetition_mean_ns",
    "repetition_median_ns", "repetition_p95_ns",
    "alloc_ops_total", "free_ops_total",
    "bytes_allocated_total", "bytes_freed_total",
    "peak_live_count", "peak_live_bytes",
    "final_live_count", "final_live_bytes",
    "ops_per_sec_mean", "throughput_bytes_per_sec_mean",
]


def _rec_to_csv_row(rec: SummaryRecord) -> dict:
    m = rec.metadata

    def _s(v) -> str:
        return "" if v is None else str(v)

    return {
        "schema_version": m.schema_version,
        "run_id": m.run_id,
        "experiment_name": m.experiment_name,
        "experiment_category": m.experiment_category,
        "allocator": m.allocator,
        "seed": _s(m.seed),
        "run_timestamp_utc": _s(m.run_timestamp_utc),
        "os_name": m.os_name,
        "os_version": m.os_version,
        "arch": m.arch,
        "compiler_name": m.compiler_name,
        "compiler_version": m.compiler_version,
        "build_type": m.build_type,
        "build_flags": m.build_flags,
        "cpu_model": m.cpu_model,
        "cpu_cores_logical": _s(m.cpu_cores_logical),
        "cpu_cores_physical": _s(m.cpu_cores_physical),
        "status": _s(rec.status),
        "failure_class": _s(rec.failure_class),
        "warmup_iterations": _s(rec.warmup_iterations),
        "measured_repetitions": _s(rec.measured_repetitions),
        "phase_duration_ns": _s(rec.phase_duration_ns),
        "repetition_min_ns": _s(rec.repetition_min_ns),
        "repetition_max_ns": _s(rec.repetition_max_ns),
        "repetition_mean_ns": _s(rec.repetition_mean_ns),
        "repetition_median_ns": _s(rec.repetition_median_ns),
        "repetition_p95_ns": _s(rec.repetition_p95_ns),
        "alloc_ops_total": _s(rec.alloc_ops_total),
        "free_ops_total": _s(rec.free_ops_total),
        "bytes_allocated_total": _s(rec.bytes_allocated_total),
        "bytes_freed_total": _s(rec.bytes_freed_total),
        "peak_live_count": _s(rec.peak_live_count),
        "peak_live_bytes": _s(rec.peak_live_bytes),
        "final_live_count": _s(rec.final_live_count),
        "final_live_bytes": _s(rec.final_live_bytes),
        "ops_per_sec_mean": _s(rec.ops_per_sec_mean),
        "throughput_bytes_per_sec_mean": _s(rec.throughput_bytes_per_sec_mean),
    }


def write_summary_csv(path: str, records: list[SummaryRecord]) -> None:
    """Write a list of SummaryRecord to a summary.v2 CSV file."""
    with open(path, "w", newline="", encoding="utf-8") as fh:
        writer = csv.DictWriter(fh, fieldnames=_SUMMARY_COLUMNS)
        writer.writeheader()
        for rec in records:
            writer.writerow(_rec_to_csv_row(rec))


def write_timeseries_jsonl(path: str, records: list[TimeSeriesRecord]) -> None:
    """Write a list of TimeSeriesRecord to a ts.v2 JSONL file."""
    with open(path, "w", encoding="utf-8") as fh:
        for rec in records:
            m = rec.metadata
            obj = {
                "schema_version": m.schema_version,
                "run_id": m.run_id,
                "experiment_name": m.experiment_name,
                "experiment_category": m.experiment_category,
                "allocator": m.allocator,
                "seed": m.seed,
                "run_timestamp_utc": m.run_timestamp_utc,
                "os_name": m.os_name,
                "os_version": m.os_version,
                "arch": m.arch,
                "compiler_name": m.compiler_name,
                "compiler_version": m.compiler_version,
                "build_type": m.build_type,
                "build_flags": m.build_flags,
                "cpu_model": m.cpu_model,
                "cpu_cores_logical": m.cpu_cores_logical,
                "cpu_cores_physical": m.cpu_cores_physical,
                "status": rec.status,
                "failure_class": rec.failure_class,
                "record_type": rec.record_type,
                "repetition_id": rec.repetition_id,
                "timestamp_ns": rec.timestamp_ns,
                "event_seq_no": rec.event_seq_no,
                "phase_name": rec.phase_name,
                "payload": rec.payload,
            }
            fh.write(json.dumps(obj) + "\n")


# ---------------------------------------------------------------------------
# Fixtures
# ---------------------------------------------------------------------------

@pytest.fixture()
def default_meta() -> RunMetadata:
    return make_metadata()


@pytest.fixture()
def default_summary_record(default_meta) -> SummaryRecord:
    return make_summary_record(meta=default_meta)


@pytest.fixture()
def simple_run(default_meta) -> RunModel:
    """A minimal valid RunModel with one summary record and no timeseries."""
    return RunModel(
        metadata=default_meta,
        summary=[make_summary_record(meta=default_meta)],
        timeseries=[],
    )


@pytest.fixture()
def two_record_run() -> RunModel:
    """A RunModel with two summary records sharing the same metadata."""
    meta = make_metadata()
    return RunModel(
        metadata=meta,
        summary=[
            make_summary_record(meta=meta, repetition_min_ns=400_000, ops_per_sec_mean=2_000_000.0),
            make_summary_record(meta=meta, repetition_min_ns=380_000, ops_per_sec_mean=2_100_000.0),
        ],
        timeseries=[],
    )


@pytest.fixture()
def run_with_na() -> RunModel:
    """A RunModel where some metric fields are None (NA)."""
    meta = make_metadata()
    return RunModel(
        metadata=meta,
        summary=[
            make_summary_record(
                meta=meta,
                ops_per_sec_mean=None,
                throughput_bytes_per_sec_mean=None,
                repetition_p95_ns=None,
            )
        ],
        timeseries=[],
    )


@pytest.fixture()
def run_pair() -> tuple[RunModel, RunModel]:
    """A (baseline, candidate) pair for comparison tests."""
    meta_base = make_metadata(run_id="run-base")
    meta_cand = make_metadata(run_id="run-cand")
    baseline = RunModel(
        metadata=meta_base,
        summary=[make_summary_record(meta=meta_base, ops_per_sec_mean=1_000_000.0,
                                     repetition_median_ns=500_000)],
        timeseries=[],
    )
    candidate = RunModel(
        metadata=meta_cand,
        summary=[make_summary_record(meta=meta_cand, ops_per_sec_mean=1_200_000.0,
                                     repetition_median_ns=420_000)],
        timeseries=[],
    )
    return baseline, candidate


@pytest.fixture()
def csv_run(tmp_path) -> tuple[str, RunModel]:
    """Write a one-record summary CSV and return (base_path, RunModel)."""
    meta = make_metadata()
    rec = make_summary_record(meta=meta)
    base = str(tmp_path / "run1")
    write_summary_csv(base + ".csv", [rec])
    run = RunModel(metadata=meta, summary=[rec], timeseries=[])
    return base, run


@pytest.fixture()
def jsonl_run(tmp_path) -> tuple[str, RunModel]:
    """Write a one-record JSONL file and return (base_path, RunModel)."""
    meta = make_metadata(schema_version="ts.v2")
    rec = make_ts_record(meta=meta)
    base = str(tmp_path / "run_ts")
    write_timeseries_jsonl(base + ".jsonl", [rec])
    run = RunModel(metadata=meta, summary=[], timeseries=[rec])
    return base, run


@pytest.fixture()
def full_run(tmp_path) -> tuple[str, RunModel]:
    """Write both CSV and JSONL files for a single run; return (base_path, RunModel)."""
    meta_s = make_metadata(schema_version="summary.v2")
    meta_t = make_metadata(schema_version="ts.v2")
    srec = make_summary_record(meta=meta_s)
    trec = make_ts_record(meta=meta_t)
    base = str(tmp_path / "run_full")
    write_summary_csv(base + ".csv", [srec])
    write_timeseries_jsonl(base + ".jsonl", [trec])
    run = RunModel(metadata=meta_s, summary=[srec], timeseries=[trec])
    return base, run

