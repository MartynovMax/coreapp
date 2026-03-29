"""
ingest_summary.py — ingestion of summary.v2 CSV files.

Responsibility:
  Read a .csv file produced by the C++ benchmark runner (schema summary.v2)
  and return a list of SummaryRecord instances.

Not yet implemented:
  - Full field parsing and NA handling.
  - Schema version validation (delegated to validate.py).
"""

from __future__ import annotations

from .models import SummaryRecord


def load_summary(path: str) -> list[SummaryRecord]:
    """
    Load a summary CSV file and return one SummaryRecord per data row.

    Args:
        path: Path to a .csv file (schema summary.v2).

    Returns:
        List of SummaryRecord instances.

    Raises:
        NotImplementedError: Full parsing is not yet implemented.
    """
    raise NotImplementedError("load_summary is not yet implemented")

