"""
validate.py — schema and content validation for ingested benchmark data.

Responsibility:
  Validate that ingested records conform to expected schemas and contain
  all required fields. Produce clear, human-readable diagnostics for every
  violation found.

  Validation rules:
  - schema_version must be a known, supported value.
  - Required identity and metadata fields must be non-empty / non-None.
  - NA values in metric fields are explicitly allowed — they represent
    unavailable measurements, not errors.
  - Unknown extra fields are tolerated (forward-compatibility).

  Validation is intentionally separate from parsing (ingest_*.py) and
  from reporting (report.py / compare.py).  Callers decide what to do
  with the returned diagnostics.

Supported schema versions:
  - summary.v2
  - ts.v2
"""

from __future__ import annotations

from dataclasses import dataclass
from typing import Optional

from .models import RunMetadata, RunModel, SummaryRecord, TimeSeriesRecord


# ---------------------------------------------------------------------------
# Supported schema versions
# ---------------------------------------------------------------------------

SUPPORTED_SUMMARY_VERSIONS: frozenset[str] = frozenset({"summary.v2"})
SUPPORTED_TIMESERIES_VERSIONS: frozenset[str] = frozenset({"ts.v2"})


# ---------------------------------------------------------------------------
# Diagnostic dataclass
# ---------------------------------------------------------------------------

@dataclass(frozen=True)
class ValidationError:
    """One validation problem found in a record or run."""

    field: str          # Field or section name, e.g. "schema_version", "metadata.run_id"
    message: str        # Human-readable description of the problem
    context: str = ""   # Optional: record index, record_type, etc.

    def __str__(self) -> str:
        parts = [f"[{self.field}] {self.message}"]
        if self.context:
            parts.insert(0, self.context)
        return " | ".join(parts)


# ---------------------------------------------------------------------------
# Shared metadata validation
# ---------------------------------------------------------------------------

# Fields that must be present and non-empty in every record's metadata.
# seed is Optional[int] in the model, so we check it separately.
_REQUIRED_METADATA_STR_FIELDS: tuple[tuple[str, str], ...] = (
    ("schema_version",     "schema_version"),
    ("run_id",             "run_id"),
    ("experiment_name",    "experiment_name"),
    ("experiment_category","experiment_category"),
    ("allocator",          "allocator"),
)


def _validate_metadata(meta: RunMetadata, context: str) -> list[ValidationError]:
    """
    Validate a RunMetadata instance and return all problems found.

    Args:
        meta:    The RunMetadata to inspect.
        context: A short string identifying the owning record (e.g. "record[0]").

    Returns:
        List of ValidationError instances; empty means no problems.
    """
    errors: list[ValidationError] = []

    # Required string fields must be non-empty.
    for attr, field_name in _REQUIRED_METADATA_STR_FIELDS:
        val: str = getattr(meta, attr, "")
        if not val:
            errors.append(ValidationError(
                field=f"metadata.{field_name}",
                message="required field is missing or empty",
                context=context,
            ))

    # seed is Optional[int] in RunMetadata; None means it was absent or
    # malformed during ingestion.  The C++ contract guarantees seed is always
    # written (u64, default 0); None therefore indicates a parse failure, not
    # a legitimately missing value.  Zero is a valid seed and must not be
    # confused with absence.
    if meta.seed is None:
        errors.append(ValidationError(
            field="metadata.seed",
            message="required field is missing or could not be parsed",
            context=context,
        ))

    return errors


# ---------------------------------------------------------------------------
# SummaryRecord validation
# ---------------------------------------------------------------------------

SUPPORTED_STATUS_VALUES: frozenset[str] = frozenset({"Valid", "Invalid"})

# Fields that are required to be non-None in a SummaryRecord.
# Metric fields are NOT listed here — NA is allowed for all metrics.
#
# `status` is kept required: RunMetadata::status defaults to RunStatus::Valid
# in C++ and is always written to every output record.  A missing or None
# status means ingestion failed to read a field that is guaranteed to exist.
#
# `warmup_iterations` and `measured_repetitions` are u32 fields written
# unconditionally by the C++ writer — their absence indicates a parse error.
_REQUIRED_SUMMARY_FIELDS: tuple[str, ...] = (
    "status",
    "warmup_iterations",
    "measured_repetitions",
)


def validate_summary_record(
    record: SummaryRecord,
    *,
    index: Optional[int] = None,
) -> list[ValidationError]:
    """
    Validate a single SummaryRecord.

    Checks:
    - schema_version is a supported summary version
    - required metadata fields are present
    - required record fields are present
    - status is a known value (when present)

    NA metric values are allowed and are not treated as errors.

    Args:
        record: The SummaryRecord to validate.
        index:  Optional row index for diagnostic context.

    Returns:
        List of ValidationError instances; empty means valid.
    """
    context = f"summary record[{index}]" if index is not None else "summary record"
    errors: list[ValidationError] = []

    # Schema version
    version = record.metadata.schema_version
    if version not in SUPPORTED_SUMMARY_VERSIONS:
        errors.append(ValidationError(
            field="schema_version",
            message=(
                f"unsupported version {version!r}; "
                f"expected one of {sorted(SUPPORTED_SUMMARY_VERSIONS)}"
            ),
            context=context,
        ))

    # Required metadata
    errors.extend(_validate_metadata(record.metadata, context))

    # Required record-level fields
    for field_name in _REQUIRED_SUMMARY_FIELDS:
        if getattr(record, field_name, None) is None:
            errors.append(ValidationError(
                field=field_name,
                message="required field is missing or could not be parsed",
                context=context,
            ))

    # Status value must be recognised when present
    if record.status is not None and record.status not in SUPPORTED_STATUS_VALUES:
        errors.append(ValidationError(
            field="status",
            message=(
                f"unrecognised value {record.status!r}; "
                f"expected one of {sorted(SUPPORTED_STATUS_VALUES)}"
            ),
            context=context,
        ))

    return errors


# ---------------------------------------------------------------------------
# TimeSeriesRecord validation
# ---------------------------------------------------------------------------

SUPPORTED_RECORD_TYPES: frozenset[str] = frozenset({"event", "tick", "warning"})

# Fields that must be non-empty / non-zero in every TimeSeriesRecord.
# timestamp_ns and event_seq_no are int fields; 0 is a valid value (first
# event), so we only check that record_type is meaningful.
_REQUIRED_TIMESERIES_STR_FIELDS: tuple[str, ...] = ("record_type",)


def validate_timeseries_record(
    record: TimeSeriesRecord,
    *,
    index: Optional[int] = None,
) -> list[ValidationError]:
    """
    Validate a single TimeSeriesRecord.

    Checks:
    - schema_version is a supported time-series version
    - required metadata fields are present
    - record_type is present and a known value

    Args:
        record: The TimeSeriesRecord to validate.
        index:  Optional line index for diagnostic context.

    Returns:
        List of ValidationError instances; empty means valid.
    """
    context = f"timeseries record[{index}]" if index is not None else "timeseries record"
    errors: list[ValidationError] = []

    # Schema version
    version = record.metadata.schema_version
    if version not in SUPPORTED_TIMESERIES_VERSIONS:
        errors.append(ValidationError(
            field="schema_version",
            message=(
                f"unsupported version {version!r}; "
                f"expected one of {sorted(SUPPORTED_TIMESERIES_VERSIONS)}"
            ),
            context=context,
        ))

    # Required metadata
    errors.extend(_validate_metadata(record.metadata, context))

    # record_type must be present
    if not record.record_type:
        errors.append(ValidationError(
            field="record_type",
            message="required field is missing or empty",
            context=context,
        ))
    elif record.record_type not in SUPPORTED_RECORD_TYPES:
        errors.append(ValidationError(
            field="record_type",
            message=(
                f"unrecognised value {record.record_type!r}; "
                f"expected one of {sorted(SUPPORTED_RECORD_TYPES)}"
            ),
            context=context,
        ))

    # Status value must be recognised when present
    if record.status is not None and record.status not in SUPPORTED_STATUS_VALUES:
        errors.append(ValidationError(
            field="status",
            message=(
                f"unrecognised value {record.status!r}; "
                f"expected one of {sorted(SUPPORTED_STATUS_VALUES)}"
            ),
            context=context,
        ))

    return errors


# ---------------------------------------------------------------------------
# RunModel validation
# ---------------------------------------------------------------------------

def validate_run(run: RunModel) -> list[ValidationError]:
    """
    Validate all records in a RunModel.

    Run-level checks (performed before per-record validation):
    - Both summary and timeseries must not both be empty.
    - All summary records must share the same schema_version.
    - All timeseries records must share the same schema_version.

    Per-record checks: every SummaryRecord and TimeSeriesRecord is
    validated individually.  All errors are collected rather than stopping
    at the first one, so callers receive a complete picture.

    Args:
        run: The RunModel to validate.

    Returns:
        List of ValidationError instances across all checks; empty means valid.
    """
    errors: list[ValidationError] = []

    # --- Run-level: must have at least some records ---
    if not run.summary and not run.timeseries:
        errors.append(ValidationError(
            field="run",
            message="run contains no records: both summary and timeseries are empty",
        ))

    # --- Run-level: summary records must agree on schema_version ---
    summary_versions = {r.metadata.schema_version for r in run.summary
                        if r.metadata.schema_version}
    if len(summary_versions) > 1:
        errors.append(ValidationError(
            field="schema_version",
            message=(
                f"inconsistent schema_version across summary records: "
                f"{sorted(summary_versions)}"
            ),
            context="summary",
        ))

    # --- Run-level: timeseries records must agree on schema_version ---
    ts_versions = {r.metadata.schema_version for r in run.timeseries
                   if r.metadata.schema_version}
    if len(ts_versions) > 1:
        errors.append(ValidationError(
            field="schema_version",
            message=(
                f"inconsistent schema_version across timeseries records: "
                f"{sorted(ts_versions)}"
            ),
            context="timeseries",
        ))

    # --- Per-record validation ---
    for i, record in enumerate(run.summary):
        errors.extend(validate_summary_record(record, index=i))

    for i, record in enumerate(run.timeseries):
        errors.extend(validate_timeseries_record(record, index=i))

    return errors


# ---------------------------------------------------------------------------
# Convenience helpers
# ---------------------------------------------------------------------------

def is_valid_summary_record(record: SummaryRecord) -> bool:
    """Return True if the record passes all summary validation checks."""
    return not validate_summary_record(record)


def is_valid_timeseries_record(record: TimeSeriesRecord) -> bool:
    """Return True if the record passes all time-series validation checks."""
    return not validate_timeseries_record(record)


def is_valid_run(run: RunModel) -> bool:
    """Return True if every record in the run passes validation."""
    return not validate_run(run)
