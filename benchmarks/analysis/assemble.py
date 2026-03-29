"""
assemble.py — assembly of a canonical RunModel from parsed and validated inputs.

Responsibility:
  Combine parsed summary records, parsed time-series records, and a
  resolved run-level RunMetadata into a single RunModel instance.

  This module sits between the ingestion layer (ingest_*.py / validate.py)
  and the reporting/comparison layer (report.py / compare.py).  Downstream
  code must consume a RunModel, never raw lists of SummaryRecord or
  TimeSeriesRecord directly.

Assembly contract:
  - Caller is responsible for ingesting and (optionally) validating records
    before calling assemble_run().
  - At least one of summary_records or timeseries_records must be non-empty;
    assemble_run() raises ValueError otherwise.
  - When both streams are non-empty, the identity fields of the first record
    in each stream are compared (run_id, experiment_name, experiment_category,
    allocator, seed).  A mismatch raises ValueError so that files from
    different runs are never silently combined into one RunModel.
  - Run-level metadata is extracted from the first available record in
    priority order: summary first, then timeseries.  All records within a
    stream are expected to share identical metadata (enforced by validate.py).
  - Explicit metadata may be supplied by the caller to override extraction
    (useful in tests or when only partial data is available).

Entry point:
  - assemble_run()       — assemble from in-memory record lists
  - assemble_run_from_files()  — load, validate, and assemble from file paths
"""

from __future__ import annotations

import warnings
from typing import Optional

from .ingest_summary import load_summary
from .ingest_timeseries import load_timeseries
from .models import RunMetadata, RunModel, SummaryRecord, TimeSeriesRecord
from .validate import validate_run


# ---------------------------------------------------------------------------
# Metadata resolution
# ---------------------------------------------------------------------------

def _extract_metadata(
    summary_records: list[SummaryRecord],
    timeseries_records: list[TimeSeriesRecord],
) -> RunMetadata:
    """
    Extract run-level RunMetadata from the first available record.

    Priority: first summary record, then first timeseries record.
    All records within a stream must share the same metadata values
    (this invariant is checked by validate_run, not here).

    Returns a default-constructed RunMetadata if no records are provided.
    """
    if summary_records:
        return summary_records[0].metadata
    if timeseries_records:
        return timeseries_records[0].metadata
    return RunMetadata()


# ---------------------------------------------------------------------------
# Cross-stream consistency check
# ---------------------------------------------------------------------------

# Identity fields that must match between the summary stream and the
# timeseries stream when both are present.  A mismatch means the two files
# were not produced by the same benchmark run and must not be combined.
_IDENTITY_FIELDS: tuple[str, ...] = (
    "run_id",
    "experiment_name",
    "experiment_category",
    "allocator",
    "seed",
)


def _check_stream_consistency(
    summary_records: list[SummaryRecord],
    timeseries_records: list[TimeSeriesRecord],
) -> None:
    """
    Verify that the first summary record and the first timeseries record
    share the same run identity metadata.

    Called only when both streams are non-empty.  Raises ValueError with
    a detailed diff if any identity field disagrees.

    Args:
        summary_records:    Non-empty list of parsed SummaryRecord.
        timeseries_records: Non-empty list of parsed TimeSeriesRecord.

    Raises:
        ValueError: If any identity field differs between the two streams.
    """
    s_meta = summary_records[0].metadata
    t_meta = timeseries_records[0].metadata

    mismatches: list[str] = []
    for field_name in _IDENTITY_FIELDS:
        s_val = getattr(s_meta, field_name)
        t_val = getattr(t_meta, field_name)
        if s_val != t_val:
            mismatches.append(
                f"  {field_name}: summary={s_val!r} vs timeseries={t_val!r}"
            )

    if mismatches:
        raise ValueError(
            "Cannot assemble RunModel: summary and timeseries streams do not "
            "belong to the same run.\n"
            "Mismatched identity fields:\n"
            + "\n".join(mismatches)
        )


# ---------------------------------------------------------------------------
# Core assembly
# ---------------------------------------------------------------------------

def assemble_run(
    summary_records: list[SummaryRecord],
    timeseries_records: list[TimeSeriesRecord],
    *,
    metadata: Optional[RunMetadata] = None,
) -> RunModel:
    """
    Assemble a RunModel from parsed record lists.

    The caller is responsible for ingesting records (via load_summary /
    load_timeseries) and, optionally, validating them (via validate_run)
    before calling this function.

    When both streams are non-empty, the identity metadata of the first
    record in each stream is compared.  A mismatch raises ValueError so
    that files from different runs are never silently combined.

    Args:
        summary_records:    Parsed SummaryRecord list (may be empty).
        timeseries_records: Parsed TimeSeriesRecord list (may be empty).
        metadata:           Optional explicit RunMetadata.  When None,
                            metadata is extracted from the first available
                            record (summary first, then timeseries).

    Returns:
        A fully assembled RunModel.

    Raises:
        ValueError: If both streams are empty, or if the streams belong to
                    different runs (identity metadata mismatch).
    """
    if not summary_records and not timeseries_records:
        raise ValueError(
            "Cannot assemble a RunModel: both summary_records and "
            "timeseries_records are empty."
        )

    if summary_records and timeseries_records:
        _check_stream_consistency(summary_records, timeseries_records)

    resolved_metadata = metadata if metadata is not None else _extract_metadata(
        summary_records, timeseries_records
    )

    return RunModel(
        metadata=resolved_metadata,
        summary=list(summary_records),
        timeseries=list(timeseries_records),
    )


# ---------------------------------------------------------------------------
# File-based entry point
# ---------------------------------------------------------------------------

def assemble_run_from_files(
    base_path: str,
    *,
    require_summary: bool = False,
    require_timeseries: bool = False,
    validate: bool = True,
) -> RunModel:
    """
    Load, optionally validate, and assemble a RunModel from output files.

    Expects files produced by the C++ benchmark runner:
      ``<base_path>.csv``   — summary output (schema ``summary.v2``)
      ``<base_path>.jsonl`` — time-series output (schema ``ts.v2``)

    Either file may be absent unless require_summary / require_timeseries
    are set.  A UserWarning is emitted for each missing optional file so
    callers can detect incomplete runs without a hard failure.

    Args:
        base_path:          Base path without extension, e.g. ``"results/run1"``.
        require_summary:    If True, raise FileNotFoundError when .csv is absent.
        require_timeseries: If True, raise FileNotFoundError when .jsonl is absent.
        validate:           If True, validate all records after ingestion and
                            raise ValueError listing all found errors when any
                            are present.

    Returns:
        An assembled RunModel.

    Raises:
        FileNotFoundError: If a required file is missing.
        ValueError:        If validation is enabled and any records fail
                           validation, or if both files are absent/empty.
    """
    summary_path = base_path + ".csv"
    timeseries_path = base_path + ".jsonl"

    summary_records = _load_optional(
        summary_path,
        load_summary,
        required=require_summary,
        label="summary",
    )
    timeseries_records = _load_optional(
        timeseries_path,
        load_timeseries,
        required=require_timeseries,
        label="timeseries",
    )

    run = assemble_run(summary_records, timeseries_records)

    if validate:
        errors = validate_run(run)
        if errors:
            lines = ["Validation failed for run assembled from " + repr(base_path) + ":"]
            lines.extend(f"  {e}" for e in errors)
            raise ValueError("\n".join(lines))

    return run


# ---------------------------------------------------------------------------
# Internal helpers
# ---------------------------------------------------------------------------

def _load_optional(
    path: str,
    loader,
    *,
    required: bool,
    label: str,
) -> list:
    """
    Attempt to load a file; handle missing files according to ``required``.

    Returns an empty list when the file is absent and not required.
    """
    import os

    if not os.path.exists(path):
        if required:
            raise FileNotFoundError(
                f"Required {label} file not found: {path!r}"
            )
        warnings.warn(
            f"{label} file not found, skipping: {path!r}",
            UserWarning,
            stacklevel=4,
        )
        return []

    return loader(path)

