"""
report.py — report generation from a single benchmark run.

Responsibility:
  Produce compact human-readable tables and/or structured output from
  an ingested RunModel. Supports filtering by allocator, experiment, or
  category. Does NOT perform comparison between runs (see compare.py).

Not yet implemented.
"""

from __future__ import annotations

from .models import RunModel


def report_summary_table(run: RunModel) -> str:
    """
    Render a compact text table of summary metrics for the given run.

    Args:
        run: An ingested and validated RunModel.

    Returns:
        Formatted string table.

    Raises:
        NotImplementedError: Not yet implemented.
    """
    raise NotImplementedError("report_summary_table is not yet implemented")


def report_run(run: RunModel) -> None:
    """
    Print a full run report to stdout.

    Args:
        run: An ingested and validated RunModel.

    Raises:
        NotImplementedError: Not yet implemented.
    """
    raise NotImplementedError("report_run is not yet implemented")

