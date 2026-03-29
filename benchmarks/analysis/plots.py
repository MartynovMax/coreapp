"""
plots.py — static plot generation for benchmark analysis.

Responsibility:
  Generate simple, deterministic charts from RunModel data for offline reports.
  All plots are written to image files (PNG by default).  No interactive
  display or GUI is used — matplotlib is run in a non-interactive backend.

  Input is always the canonical RunModel (or a comparison/filtered subset).
  Plotting logic must not perform ingestion, validation, or comparison.

Filtering scope
---------------
  RunFilter (from filters.py) operates on SummaryRecord fields
  (allocator, workload, profile, experiment_name).

  plot_latency_summary and plot_throughput_comparison consume run.summary,
  so a pre-filtered RunModel (via filters.apply_filter) restricts those plots
  to the matching subset without any extra work.

  plot_timeseries_trend consumes run.timeseries.  TimeSeriesRecord carries
  the same metadata fields (experiment_name, allocator) so it accepts an
  optional ``timeseries_filter`` parameter (RunFilter) which narrows which
  (experiment_name, allocator) groups are plotted.  Pass a RunFilter to
  restrict the timeseries plot to the same scope as a filtered summary plot.

Available plots:
  plot_latency_summary(run, output_path)
      Grouped bar chart of min / median / p95 / max latency per record.
      One group per record, grouped and colour-coded by metric.
      Pass a pre-filtered RunModel to restrict the scope.

  plot_throughput_comparison(runs_by_label, output_path)
      Grouped bar chart of mean ops/sec across multiple runs or allocators.
      Each run / label is one bar group.
      Pass pre-filtered RunModels to restrict the scope.

  plot_timeseries_trend(run, metric_key, output_path, *, timeseries_filter)
      Line chart of a numeric payload field vs timestamp_ns from JSONL
      tick records.  One line per unique (experiment_name, allocator) pair.
      Use ``timeseries_filter`` to restrict which groups are plotted.

Dependencies:
  matplotlib  (required)
  numpy       (required, pulled in transitively by matplotlib)
"""

from __future__ import annotations

import warnings
from typing import Optional

import matplotlib
matplotlib.use("Agg")          # non-interactive backend — safe for offline use
import matplotlib.pyplot as plt
import matplotlib.ticker as mticker
import numpy as np

from .models import RunModel, SummaryRecord, TimeSeriesRecord
from .filters import RunFilter


# ---------------------------------------------------------------------------
# Internal helpers
# ---------------------------------------------------------------------------

_NA_LABEL = "NA"
_DEFAULT_FIGSIZE = (10, 5)
_BAR_ALPHA = 0.85


def _ns_to_label(value_ns: Optional[float]) -> str:
    """Convert a nanosecond value to a human-readable string with unit."""
    if value_ns is None:
        return _NA_LABEL
    v = float(value_ns)
    if v >= 1_000_000_000:
        return f"{v / 1_000_000_000:.2f} s"
    if v >= 1_000_000:
        return f"{v / 1_000_000:.2f} ms"
    if v >= 1_000:
        return f"{v / 1_000:.2f} µs"
    return f"{v:.0f} ns"


def _ns_axis_unit(values_ns: list[float]) -> tuple[float, str]:
    """
    Choose a sensible time unit for an axis given a list of nanosecond values.

    Returns (divisor, unit_label).
    """
    if not values_ns:
        return 1.0, "ns"
    max_v = max(values_ns)
    if max_v >= 1_000_000_000:
        return 1_000_000_000.0, "s"
    if max_v >= 1_000_000:
        return 1_000_000.0, "ms"
    if max_v >= 1_000:
        return 1_000.0, "µs"
    return 1.0, "ns"


def _save_fig(fig: plt.Figure, output_path: str, dpi: int = 150) -> None:
    """Save a figure to a file and close it."""
    fig.savefig(output_path, dpi=dpi, bbox_inches="tight")
    plt.close(fig)


def _record_label(record: SummaryRecord, index: int) -> str:
    """Build a short display label for one SummaryRecord."""
    exp = record.metadata.experiment_name or f"rec{index}"
    alloc = record.metadata.allocator
    if alloc:
        return f"{exp}\n({alloc})"
    return exp


# ---------------------------------------------------------------------------
# Plot 1: Latency summary for a single run
# ---------------------------------------------------------------------------

def plot_latency_summary(
    run: RunModel,
    output_path: str,
    *,
    dpi: int = 150,
    figsize: tuple[int, int] = _DEFAULT_FIGSIZE,
) -> None:
    """
    Write a grouped bar chart of latency metrics for a single run.

    For each summary record four bars are plotted side by side:
      min / median / p95 / max (all in nanoseconds, scaled to the best unit).

    Records where all four latency fields are NA are skipped with a warning.
    NA values within a record are rendered as zero-height bars so the layout
    remains stable — a text annotation marks them as "NA".

    Args:
        run:         An assembled RunModel (may be pre-filtered).
        output_path: Destination image path (e.g. "latency.png").
        dpi:         Output resolution.
        figsize:     Figure dimensions in inches.

    Raises:
        ValueError: If run.summary is empty or all records have no latency data.
    """
    if not run.summary:
        raise ValueError("plot_latency_summary: run.summary is empty")

    _LATENCY_FIELDS = [
        ("repetition_min_ns",    "min",    "#4C72B0"),
        ("repetition_median_ns", "median", "#55A868"),
        ("repetition_p95_ns",    "p95",    "#DD8452"),
        ("repetition_max_ns",    "max",    "#C44E52"),
    ]

    # Collect data — skip records with no latency at all.
    labels: list[str] = []
    data: list[list[Optional[float]]] = []   # data[field_idx][record_idx]
    for _ in _LATENCY_FIELDS:
        data.append([])

    for idx, rec in enumerate(run.summary):
        values = [getattr(rec, f, None) for f, _, _ in _LATENCY_FIELDS]
        if all(v is None for v in values):
            warnings.warn(
                f"plot_latency_summary: record {idx} has no latency data — skipped",
                UserWarning,
                stacklevel=2,
            )
            continue
        labels.append(_record_label(rec, idx))
        for fi, v in enumerate(values):
            data[fi].append(float(v) if v is not None else None)

    if not labels:
        raise ValueError("plot_latency_summary: no records with latency data")

    # Choose time unit from all available non-None values.
    all_vals = [v for series in data for v in series if v is not None]
    divisor, unit = _ns_axis_unit(all_vals)

    n_records = len(labels)
    n_metrics = len(_LATENCY_FIELDS)
    x = np.arange(n_records)
    bar_w = 0.18
    offsets = np.linspace(-(n_metrics - 1) / 2, (n_metrics - 1) / 2, n_metrics) * bar_w

    fig, ax = plt.subplots(figsize=figsize)

    for fi, ((_fname, metric_label, colour), offset) in enumerate(
        zip(_LATENCY_FIELDS, offsets)
    ):
        heights = [
            (v / divisor if v is not None else 0.0) for v in data[fi]
        ]
        bars = ax.bar(
            x + offset, heights, bar_w,
            label=metric_label, color=colour, alpha=_BAR_ALPHA,
        )
        # Mark NA bars explicitly.
        for bar, v in zip(bars, data[fi]):
            if v is None:
                ax.text(
                    bar.get_x() + bar.get_width() / 2,
                    0.0,
                    _NA_LABEL,
                    ha="center", va="bottom", fontsize=6, color="grey",
                )

    ax.set_xticks(x)
    ax.set_xticklabels(labels, fontsize=8)
    ax.set_ylabel(f"Latency ({unit})")
    ax.set_title(
        f"Latency Summary — {run.metadata.run_id or 'run'}"
        if run.metadata.run_id else "Latency Summary"
    )
    ax.legend(loc="upper right", fontsize=8)
    ax.yaxis.set_major_formatter(mticker.FormatStrFormatter("%.2f"))
    ax.grid(axis="y", linestyle="--", alpha=0.4)
    fig.tight_layout()

    _save_fig(fig, output_path, dpi=dpi)


# ---------------------------------------------------------------------------
# Plot 2: Throughput comparison across multiple runs / allocators
# ---------------------------------------------------------------------------

def plot_throughput_comparison(
    runs_by_label: dict[str, RunModel],
    output_path: str,
    *,
    metric: str = "ops_per_sec_mean",
    dpi: int = 150,
    figsize: tuple[int, int] = _DEFAULT_FIGSIZE,
) -> None:
    """
    Write a grouped bar chart comparing a throughput-style metric across runs.

    One bar group per experiment_name.  Within each group one bar per label
    (key in *runs_by_label*).  Values are averaged over all summary records
    that share the same experiment_name within a run.

    NA records are excluded from the average.  If all records for a
    (label, experiment) pair are NA the bar is omitted and a warning is emitted.

    Args:
        runs_by_label: Ordered dict mapping display label → RunModel.
                       Use collections.OrderedDict or a regular dict (Python 3.7+
                       preserves insertion order).
        output_path:   Destination image path.
        metric:        SummaryRecord field to plot.  Defaults to
                       ``ops_per_sec_mean``.  Must be a numeric field.
        dpi:           Output resolution.
        figsize:       Figure dimensions in inches.

    Raises:
        ValueError: If runs_by_label is empty, or no usable data was found.
    """
    if not runs_by_label:
        raise ValueError("plot_throughput_comparison: runs_by_label is empty")

    # Gather all experiment names across all runs, stable-sorted.
    all_experiments: list[str] = sorted({
        rec.metadata.experiment_name
        for run in runs_by_label.values()
        for rec in run.summary
        if rec.metadata.experiment_name
    })
    if not all_experiments:
        raise ValueError("plot_throughput_comparison: no experiment names found in summary records")

    run_labels = list(runs_by_label.keys())
    n_exps = len(all_experiments)
    n_labels = len(run_labels)

    # Build matrix: data[label_idx][exp_idx] = mean value or None
    matrix: list[list[Optional[float]]] = [
        [None] * n_exps for _ in run_labels
    ]

    for li, label in enumerate(run_labels):
        run = runs_by_label[label]
        for ei, exp in enumerate(all_experiments):
            vals = [
                float(getattr(rec, metric))
                for rec in run.summary
                if rec.metadata.experiment_name == exp
                and getattr(rec, metric, None) is not None
            ]
            if vals:
                matrix[li][ei] = sum(vals) / len(vals)
            else:
                warnings.warn(
                    f"plot_throughput_comparison: no data for "
                    f"label={label!r} experiment={exp!r} metric={metric!r} — skipped",
                    UserWarning,
                    stacklevel=2,
                )

    # Check that at least one cell has data.
    if all(v is None for row in matrix for v in row):
        raise ValueError(
            f"plot_throughput_comparison: no usable data for metric {metric!r}"
        )

    # Colour cycle — one colour per run label.
    cmap = plt.get_cmap("tab10")
    colours = [cmap(i % 10) for i in range(n_labels)]

    x = np.arange(n_exps)
    bar_w = min(0.7 / max(n_labels, 1), 0.25)
    offsets = np.linspace(-(n_labels - 1) / 2, (n_labels - 1) / 2, n_labels) * bar_w

    fig, ax = plt.subplots(figsize=figsize)

    # Compute the reference height used for NA placeholder bars:
    # 5 % of the maximum non-None value, so NA bars are clearly visible
    # yet visually distinct from any real measurement.
    all_real_vals = [v for row in matrix for v in row if v is not None]
    na_bar_height = max(all_real_vals) * 0.05 if all_real_vals else 1.0

    for li, (label, colour, offset) in enumerate(zip(run_labels, colours, offsets)):
        for ei in range(n_exps):
            bar_x = x[ei] + offset
            value = matrix[li][ei]

            if value is not None:
                ax.bar(bar_x, value, bar_w, color=colour, alpha=_BAR_ALPHA, label=label if ei == 0 else None)
            else:
                # Draw a clearly distinct placeholder: hatched, low-opacity, grey-edged.
                ax.bar(
                    bar_x, na_bar_height, bar_w,
                    color=colour, alpha=0.20,
                    hatch="///", edgecolor="grey", linewidth=0.6,
                    label=None,
                )
                ax.text(
                    bar_x, na_bar_height * 1.05,
                    _NA_LABEL,
                    ha="center", va="bottom", fontsize=7, color="grey",
                    fontstyle="italic",
                )

    ax.set_xticks(x)
    ax.set_xticklabels(all_experiments, fontsize=8, rotation=15, ha="right")
    ax.set_ylabel(metric.replace("_", " "))
    ax.set_title("Throughput Comparison")
    ax.legend(loc="upper right", fontsize=8)
    ax.yaxis.set_major_formatter(mticker.FormatStrFormatter("%.0f"))
    ax.grid(axis="y", linestyle="--", alpha=0.4)
    fig.tight_layout()

    _save_fig(fig, output_path, dpi=dpi)


# ---------------------------------------------------------------------------
# Plot 3: Time-series trend from JSONL tick records
# ---------------------------------------------------------------------------

def plot_timeseries_trend(
    run: RunModel,
    metric_key: str,
    output_path: str,
    *,
    record_type: str = "tick",
    timeseries_filter: Optional[RunFilter] = None,
    dpi: int = 150,
    figsize: tuple[int, int] = _DEFAULT_FIGSIZE,
) -> None:
    """
    Write a line chart of a numeric payload field vs timestamp_ns.

    Only JSONL records whose ``record_type`` matches *record_type* (default
    ``"tick"``) are included.  One line is drawn per unique
    (experiment_name, allocator) pair, labelled accordingly.

    The X-axis shows elapsed time from the first timestamp in the run
    (scaled to a sensible unit).  The Y-axis shows the raw numeric value
    of *metric_key* from the record payload dict.

    Non-numeric or missing payload values are silently skipped (the line
    may have gaps).

    Filtering:
        ``timeseries_filter`` accepts a RunFilter and restricts which
        (experiment_name, allocator) groups are plotted.  Only the
        ``experiments`` and ``allocators`` dimensions of the filter are
        meaningful here (``workloads`` and ``profiles`` are summary-only
        fields that timeseries records do not carry).  Pass the same
        RunFilter used for a filtered summary plot to keep both views
        consistent.

    Args:
        run:                An assembled RunModel with timeseries records.
        metric_key:         Key inside the ``payload`` dict to plot on the Y-axis.
        output_path:        Destination image path.
        record_type:        Only records with this ``record_type`` are plotted.
                            Defaults to ``"tick"``.
        timeseries_filter:  Optional RunFilter; only groups whose
                            experiment_name / allocator satisfy the filter
                            are plotted.  Pass None to plot all groups.
        dpi:                Output resolution.
        figsize:            Figure dimensions in inches.

    Raises:
        ValueError: If run.timeseries is empty, or no usable records found.
    """
    if not run.timeseries:
        raise ValueError("plot_timeseries_trend: run.timeseries is empty")

    # Filter to the requested record_type.
    tick_records = [r for r in run.timeseries if r.record_type == record_type]
    if not tick_records:
        raise ValueError(
            f"plot_timeseries_trend: no records with record_type={record_type!r}"
        )

    # Group by (experiment_name, allocator).
    groups: dict[tuple[str, str], list[TimeSeriesRecord]] = {}
    for rec in tick_records:
        key = (rec.metadata.experiment_name, rec.metadata.allocator)
        groups.setdefault(key, []).append(rec)

    # Apply timeseries_filter: restrict to allowed experiments / allocators.
    # workloads and profiles are summary-only dimensions and are ignored here.
    if timeseries_filter is not None:
        allowed_exps   = set(timeseries_filter.experiments) if timeseries_filter.experiments else None
        allowed_allocs = set(timeseries_filter.allocators)  if timeseries_filter.allocators  else None
        groups = {
            (exp, alloc): recs
            for (exp, alloc), recs in groups.items()
            if (allowed_exps   is None or exp   in allowed_exps)
            and (allowed_allocs is None or alloc in allowed_allocs)
        }

    if not groups:
        raise ValueError(
            "plot_timeseries_trend: no groups remain after applying timeseries_filter"
        )

    # Sort each group by timestamp_ns.
    for recs in groups.values():
        recs.sort(key=lambda r: r.timestamp_ns)

    # Determine global t0 for elapsed time.
    all_ts = [r.timestamp_ns for recs in groups.values() for r in recs]
    t0 = min(all_ts)

    elapsed_ns = [float(ts - t0) for ts in all_ts]
    divisor, unit = _ns_axis_unit(elapsed_ns if elapsed_ns else [0.0])

    cmap = plt.get_cmap("tab10")
    sorted_keys = sorted(groups.keys())

    fig, ax = plt.subplots(figsize=figsize)
    plotted_any = False

    for gi, gkey in enumerate(sorted_keys):
        exp, alloc = gkey
        recs = groups[gkey]
        xs: list[float] = []
        ys: list[float] = []
        for rec in recs:
            raw = rec.payload.get(metric_key)
            try:
                ys.append(float(raw))
                xs.append((rec.timestamp_ns - t0) / divisor)
            except (TypeError, ValueError):
                continue  # skip non-numeric / missing values

        if not xs:
            warnings.warn(
                f"plot_timeseries_trend: no numeric values for "
                f"metric_key={metric_key!r} in group ({exp!r}, {alloc!r}) — skipped",
                UserWarning,
                stacklevel=2,
            )
            continue

        label = f"{exp} / {alloc}" if alloc else exp
        ax.plot(
            xs, ys,
            label=label,
            color=cmap(gi % 10),
            linewidth=1.2,
            marker="o",
            markersize=3,
            alpha=0.85,
        )
        plotted_any = True

    if not plotted_any:
        plt.close(fig)
        raise ValueError(
            f"plot_timeseries_trend: no usable data found for metric_key={metric_key!r}"
        )

    ax.set_xlabel(f"Elapsed time ({unit})")
    ax.set_ylabel(metric_key.replace("_", " "))
    ax.set_title(f"Time-Series Trend — {metric_key}")
    ax.legend(loc="best", fontsize=8)
    ax.grid(linestyle="--", alpha=0.4)
    fig.tight_layout()

    _save_fig(fig, output_path, dpi=dpi)
