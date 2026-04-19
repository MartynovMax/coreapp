"""
ingest_summary.py — ingestion of summary.v2 CSV files.

Responsibility:
  Read a .csv file produced by the C++ benchmark runner (schema summary.v2)
  and return a list of SummaryRecord instances.

  Parsing rules:
  - Header-based column mapping (never positional).
  - Empty CSV cells are preserved as None (NA), never coerced to 0.
  - Malformed numeric cells (non-empty but unparseable) are also None;
    the raw value is not silently discarded — a warning is attached so
    a later validation layer can detect and report it.
  - Unknown columns are silently ignored.  The C++ schema may add new
    optional fields in future versions; ignoring extras keeps the reader
    forward-compatible without requiring a version bump here.
  - Schema version validation is intentionally left to validate.py.
"""

from __future__ import annotations

import csv
import warnings
from typing import Optional

from .models import RunMetadata, SummaryRecord


# ---------------------------------------------------------------------------
# Internal cell-parsing helpers
#
# Each helper returns None for two distinct cases:
#   (a) empty / whitespace-only cell  — legitimate NA from C++ output
#   (b) non-empty but unparseable     — malformed data; a UserWarning is
#       emitted so a validation layer can later surface these cases
#
# Keeping the two cases visually separate in helpers makes it easier to
# extend them later (e.g. return a sentinel instead of None for malformed).
# ---------------------------------------------------------------------------

def _parse_str(raw: str) -> Optional[str]:
    """Strip whitespace; return None for empty cells."""
    stripped = raw.strip()
    return stripped if stripped else None


def _parse_int(raw: str, column: str = "") -> Optional[int]:
    """
    Parse an integer cell.

    Returns None for empty cells (NA) and for cells that cannot be parsed
    as an integer.  A UserWarning is issued for the malformed case so that
    a validation layer can detect it without this function raising.
    """
    stripped = raw.strip()
    if not stripped:
        return None
    try:
        # C++ writes u64/u32 integers; int() handles arbitrarily large values.
        return int(stripped)
    except ValueError:
        warnings.warn(
            f"Column {column!r}: expected integer, got {stripped!r} — treating as NA",
            UserWarning,
            stacklevel=3,
        )
        return None


def _parse_float(raw: str, column: str = "") -> Optional[float]:
    """
    Parse a float cell.

    Returns None for empty cells (NA) and for cells that cannot be parsed
    as a float.  A UserWarning is issued for the malformed case.
    """
    stripped = raw.strip()
    if not stripped:
        return None
    try:
        return float(stripped)
    except ValueError:
        warnings.warn(
            f"Column {column!r}: expected float, got {stripped!r} — treating as NA",
            UserWarning,
            stacklevel=3,
        )
        return None


def _parse_env_str(raw: str, fallback: str) -> str:
    """
    Parse an environment metadata string cell.

    C++ guarantees a non-empty fallback (e.g. "unknown") for env fields, but
    the cell may still be empty if the column is missing entirely or was blank.
    In that case, use the provided fallback rather than storing an empty string.
    """
    stripped = raw.strip()
    return stripped if stripped else fallback


# ---------------------------------------------------------------------------
# Metadata extraction
# ---------------------------------------------------------------------------

def _parse_metadata(row: dict[str, str]) -> RunMetadata:
    """Map CSV columns to a RunMetadata instance."""
    return RunMetadata(
        schema_version=_parse_str(row.get("schema_version", "")) or "",
        run_id=_parse_str(row.get("run_id", "")) or "",
        experiment_name=_parse_str(row.get("experiment_name", "")) or "",
        experiment_category=_parse_str(row.get("experiment_category", "")) or "",
        allocator=_parse_str(row.get("allocator", "")) or "",
        # seed is Optional[int]: keep None if absent/malformed; do not default to 0.
        seed=_parse_int(row.get("seed", ""), "seed"),
        run_timestamp_utc=_parse_str(row.get("run_timestamp_utc", "")),
        # Env strings: empty cell → fallback "unknown" (matches C++ guarantee).
        os_name=_parse_env_str(row.get("os_name", ""), "unknown"),
        os_version=_parse_env_str(row.get("os_version", ""), "unknown"),
        arch=_parse_env_str(row.get("arch", ""), "unknown"),
        compiler_name=_parse_env_str(row.get("compiler_name", ""), "unknown"),
        compiler_version=_parse_env_str(row.get("compiler_version", ""), "unknown"),
        build_type=_parse_env_str(row.get("build_type", ""), "unknown"),
        build_flags=_parse_env_str(row.get("build_flags", ""), "unknown"),
        cpu_model=_parse_env_str(row.get("cpu_model", ""), "unknown"),
        # CPU core counts are Optional[int]: keep None if absent/malformed.
        cpu_cores_logical=_parse_int(row.get("cpu_cores_logical", ""), "cpu_cores_logical"),
        cpu_cores_physical=_parse_int(row.get("cpu_cores_physical", ""), "cpu_cores_physical"),
    )


# ---------------------------------------------------------------------------
# Record extraction
# ---------------------------------------------------------------------------

def _parse_record(row: dict[str, str]) -> SummaryRecord:
    """Map one CSV row to a SummaryRecord."""
    return SummaryRecord(
        metadata=_parse_metadata(row),

        status=_parse_str(row.get("status", "")),
        failure_class=_parse_str(row.get("failure_class", "")),

        # workload and profile are not in the current summary.v2 C++ schema.
        # Parsed opportunistically so the ingestion layer is forward-compatible
        # with future schema additions; remain None when the column is absent.
        workload=_parse_str(row.get("workload", "")),
        profile=_parse_str(row.get("profile", "")),

        warmup_iterations=_parse_int(row.get("warmup_iterations", ""), "warmup_iterations"),
        measured_repetitions=_parse_int(row.get("measured_repetitions", ""), "measured_repetitions"),

        # Timer metrics — NA preserved as None
        phase_duration_ns=_parse_int(row.get("phase_duration_ns", ""), "phase_duration_ns"),
        repetition_min_ns=_parse_int(row.get("repetition_min_ns", ""), "repetition_min_ns"),
        repetition_max_ns=_parse_int(row.get("repetition_max_ns", ""), "repetition_max_ns"),
        repetition_mean_ns=_parse_float(row.get("repetition_mean_ns", ""), "repetition_mean_ns"),
        repetition_median_ns=_parse_int(row.get("repetition_median_ns", ""), "repetition_median_ns"),
        repetition_p95_ns=_parse_int(row.get("repetition_p95_ns", ""), "repetition_p95_ns"),

        # Counter metrics — NA preserved as None
        alloc_ops_total=_parse_int(row.get("alloc_ops_total", ""), "alloc_ops_total"),
        free_ops_total=_parse_int(row.get("free_ops_total", ""), "free_ops_total"),
        bytes_allocated_total=_parse_int(row.get("bytes_allocated_total", ""), "bytes_allocated_total"),
        bytes_freed_total=_parse_int(row.get("bytes_freed_total", ""), "bytes_freed_total"),
        peak_live_count=_parse_int(row.get("peak_live_count", ""), "peak_live_count"),
        peak_live_bytes=_parse_int(row.get("peak_live_bytes", ""), "peak_live_bytes"),
        final_live_count=_parse_int(row.get("final_live_count", ""), "final_live_count"),
        final_live_bytes=_parse_int(row.get("final_live_bytes", ""), "final_live_bytes"),

        # Derived metrics — NA preserved as None
        ops_per_sec_mean=_parse_float(row.get("ops_per_sec_mean", ""), "ops_per_sec_mean"),
        throughput_bytes_per_sec_mean=_parse_float(
            row.get("throughput_bytes_per_sec_mean", ""), "throughput_bytes_per_sec_mean"
        ),

        # Extended counter metrics (summary.v5+) — opportunistically parsed
        failed_alloc_count=_parse_int(row.get("failed_alloc_count", ""), "failed_alloc_count"),
        fallback_count=_parse_int(row.get("fallback_count", ""), "fallback_count"),
        reserved_bytes=_parse_int(row.get("reserved_bytes", ""), "reserved_bytes"),
        overhead_ratio=_parse_float(row.get("overhead_ratio", ""), "overhead_ratio"),
        overhead_ratio_req=_parse_float(row.get("overhead_ratio_req", ""), "overhead_ratio_req"),
    )


# ---------------------------------------------------------------------------
# Public API
# ---------------------------------------------------------------------------

def load_summary(path: str) -> list[SummaryRecord]:
    """
    Load a summary CSV file and return one SummaryRecord per data row.

    Parsing rules:
    - Columns are matched by header name, not by position.
    - Empty cells (NA in C++ output) are preserved as None.
    - Non-empty but unparseable numeric cells are also None; a UserWarning
      is emitted for each, allowing an upstream validation layer to surface
      them without this function raising.
    - Unknown columns are ignored (forward-compatible with schema additions).
    - Schema version checking is delegated to validate.py.

    Args:
        path: Path to a .csv file produced with ``--format=summary`` or
              ``--format=all`` (schema ``summary.v2``).

    Returns:
        List of SummaryRecord instances, one per data row.

    Raises:
        FileNotFoundError: If ``path`` does not exist.
        ValueError: If the file contains no header row.
    """
    records: list[SummaryRecord] = []

    with open(path, newline="", encoding="utf-8") as fh:
        reader = csv.DictReader(fh)

        # Force header reading before iterating.  DictReader populates
        # fieldnames lazily on first access; reading it here lets us
        # detect a completely empty file before any row processing.
        if not reader.fieldnames:
            raise ValueError(f"No header row found in {path!r}")

        for row in reader:
            records.append(_parse_record(row))

    return records
