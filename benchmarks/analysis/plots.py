"""
plots.py — plotting support for benchmark analysis.

Responsibility:
  Generate charts and figures from ingested RunModel data.
  Intended for offline use; no interactive dashboards.

  Planned dependencies (not yet required at skeleton stage):
    - matplotlib
    - pandas (optional, for data manipulation)

Not yet implemented.
"""

from __future__ import annotations

from .models import RunModel


def plot_latency_distribution(run: RunModel, output_path: str) -> None:
    """
    Plot the distribution of repetition latencies (min/median/p95/max).

    Args:
        run:         An ingested and validated RunModel.
        output_path: File path for the output image (e.g. "latency.png").

    Raises:
        NotImplementedError: Not yet implemented.
    """
    raise NotImplementedError("plot_latency_distribution is not yet implemented")


def plot_throughput_comparison(runs: list[RunModel], output_path: str) -> None:
    """
    Plot throughput (ops/sec) across multiple runs side by side.

    Args:
        runs:        List of RunModel instances to compare.
        output_path: File path for the output image.

    Raises:
        NotImplementedError: Not yet implemented.
    """
    raise NotImplementedError("plot_throughput_comparison is not yet implemented")

