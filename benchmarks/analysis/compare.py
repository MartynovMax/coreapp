"""
compare.py — run-to-run comparison of benchmark results.

Responsibility:
  Compare two RunModel instances and produce a diff report for key metrics.
  NA metrics must be handled safely: NA is never treated as zero, and
  comparisons involving NA values are explicitly flagged.

Not yet implemented.
"""

from __future__ import annotations

from .models import RunModel


def compare_runs(baseline: RunModel, candidate: RunModel) -> dict:
    """
    Compare two runs and return a structured diff of key metrics.

    NA values in either run are propagated explicitly — they are never
    coerced to zero.

    Args:
        baseline:  The reference run.
        candidate: The run being evaluated against the baseline.

    Returns:
        Dict with per-metric deltas and NA flags. Schema TBD.

    Raises:
        NotImplementedError: Not yet implemented.
    """
    raise NotImplementedError("compare_runs is not yet implemented")


def format_comparison(diff: dict) -> str:
    """
    Render a comparison diff dict as a human-readable text report.

    Args:
        diff: Output of compare_runs().

    Returns:
        Formatted string report.

    Raises:
        NotImplementedError: Not yet implemented.
    """
    raise NotImplementedError("format_comparison is not yet implemented")

