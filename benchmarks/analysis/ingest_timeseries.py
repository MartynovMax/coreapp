"""
ingest_timeseries.py — ingestion of ts.v2 JSONL files.

Responsibility:
  Read a .jsonl file produced by the C++ benchmark runner (schema ts.v2)
  and return a list of TimeSeriesRecord instances.

Not yet implemented:
  - Full field parsing per record_type (event / tick / warning).
  - Schema version validation (delegated to validate.py).
"""

from __future__ import annotations

from .models import TimeSeriesRecord


def load_timeseries(path: str) -> list[TimeSeriesRecord]:
    """
    Load a time-series JSONL file and return one TimeSeriesRecord per line.

    Args:
        path: Path to a .jsonl file (schema ts.v2).

    Returns:
        List of TimeSeriesRecord instances.

    Raises:
        NotImplementedError: Full parsing is not yet implemented.
    """
    raise NotImplementedError("load_timeseries is not yet implemented")

