"""
validate.py — schema and content validation for ingested benchmark data.

Responsibility:
  Validate that ingested records conform to expected schemas and contain
  all required fields. Detect schema version mismatches, missing required
  fields, and unsafe NA handling (NA must not be treated as zero).

Not yet implemented.
"""

from __future__ import annotations

from .models import RunModel, SummaryRecord, TimeSeriesRecord


def validate_summary_record(record: SummaryRecord) -> list[str]:
    """
    Validate a single SummaryRecord.

    Returns:
        List of validation error messages. Empty list means valid.

    Raises:
        NotImplementedError: Not yet implemented.
    """
    raise NotImplementedError("validate_summary_record is not yet implemented")


def validate_timeseries_record(record: TimeSeriesRecord) -> list[str]:
    """
    Validate a single TimeSeriesRecord.

    Returns:
        List of validation error messages. Empty list means valid.

    Raises:
        NotImplementedError: Not yet implemented.
    """
    raise NotImplementedError("validate_timeseries_record is not yet implemented")


def validate_run(run: RunModel) -> list[str]:
    """
    Validate all records in a RunModel.

    Returns:
        List of validation error messages across all records.

    Raises:
        NotImplementedError: Not yet implemented.
    """
    raise NotImplementedError("validate_run is not yet implemented")

