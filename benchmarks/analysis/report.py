"""
report.py — compact report generation for a single benchmark run.

Responsibility:
  Convert a RunModel into human-readable text output.

  Output is deterministic: fields and rows are always emitted in the
  same order regardless of the input data.

  NA values (None in the model) are displayed as the literal string "NA"
  and are never substituted with zeros or omitted.

  Formatting logic lives here.  Data logic lives in models.py,
  ingest_*.py, validate.py, and assemble.py.

Public API:
  report_run(run)            — full report as a string (metadata + table)
  report_metadata(run)       — metadata section only
  report_summary_table(run)  — summary metrics table only
  print_run(run)             — convenience: print report_run() to stdout
"""

from __future__ import annotations

from typing import Optional

from .models import RunMetadata, RunModel, SummaryRecord


# ---------------------------------------------------------------------------
# NA sentinel
# ---------------------------------------------------------------------------

_NA = "NA"


def _fmt_int(value: Optional[int]) -> str:
    return _NA if value is None else str(value)


def _fmt_float(value: Optional[float], precision: int = 2) -> str:
    return _NA if value is None else f"{value:.{precision}f}"


def _fmt_str(value: Optional[str], fallback: str = _NA) -> str:
    if not value:
        return fallback
    return value


def _fmt_ns(value_ns: Optional[int]) -> str:
    """Format a nanosecond integer as a human-readable string with unit."""
    if value_ns is None:
        return _NA
    if value_ns >= 1_000_000_000:
        return f"{value_ns / 1_000_000_000:.3f} s"
    if value_ns >= 1_000_000:
        return f"{value_ns / 1_000_000:.3f} ms"
    if value_ns >= 1_000:
        return f"{value_ns / 1_000:.3f} us"
    return f"{value_ns} ns"


def _fmt_bytes(value: Optional[int]) -> str:
    """Format a byte count as a human-readable string with unit."""
    if value is None:
        return _NA
    if value >= 1_073_741_824:
        return f"{value / 1_073_741_824:.2f} GiB"
    if value >= 1_048_576:
        return f"{value / 1_048_576:.2f} MiB"
    if value >= 1_024:
        return f"{value / 1_024:.2f} KiB"
    return f"{value} B"


# ---------------------------------------------------------------------------
# Simple table renderer (no external dependencies)
# ---------------------------------------------------------------------------

def _render_table(rows: list[tuple[str, str]], header: tuple[str, str] = ("Field", "Value")) -> str:
    """
    Render a two-column key/value table as a plain Markdown table.

    Column widths are determined by the widest cell in each column so
    the output is stable and aligned.
    """
    col0_w = max(len(header[0]), max((len(r[0]) for r in rows), default=0))
    col1_w = max(len(header[1]), max((len(r[1]) for r in rows), default=0))

    sep = f"| {'-' * col0_w} | {'-' * col1_w} |"
    head = f"| {header[0]:<{col0_w}} | {header[1]:<{col1_w}} |"
    lines = [head, sep]
    for k, v in rows:
        lines.append(f"| {k:<{col0_w}} | {v:<{col1_w}} |")
    return "\n".join(lines)


def _render_multi_col_table(
    headers: list[str],
    rows: list[list[str]],
) -> str:
    """
    Render a multi-column Markdown table.

    All rows must have the same number of columns as headers.
    Column widths are computed from the widest cell per column.
    """
    n = len(headers)
    widths = [len(h) for h in headers]
    for row in rows:
        for i, cell in enumerate(row):
            widths[i] = max(widths[i], len(cell))

    def fmt_row(cells: list[str]) -> str:
        return "| " + " | ".join(f"{c:<{widths[i]}}" for i, c in enumerate(cells)) + " |"

    sep = "| " + " | ".join("-" * w for w in widths) + " |"
    lines = [fmt_row(headers), sep]
    for row in rows:
        lines.append(fmt_row(row))
    return "\n".join(lines)


# ---------------------------------------------------------------------------
# Section: Run metadata
# ---------------------------------------------------------------------------

def report_metadata(run: RunModel) -> str:
    """
    Render the run metadata section as a Markdown key/value table.

    Args:
        run: An assembled RunModel.

    Returns:
        Formatted string section.
    """
    m: RunMetadata = run.metadata

    rows: list[tuple[str, str]] = [
        ("run_id",              _fmt_str(m.run_id)),
        ("experiment_name",     _fmt_str(m.experiment_name)),
        ("experiment_category", _fmt_str(m.experiment_category)),
        ("allocator",           _fmt_str(m.allocator)),
        ("seed",                _fmt_int(m.seed)),
        ("schema_version",      _fmt_str(m.schema_version)),
        ("run_timestamp_utc",   _fmt_str(m.run_timestamp_utc)),
        ("os_name",             _fmt_str(m.os_name, "unknown")),
        ("os_version",          _fmt_str(m.os_version, "unknown")),
        ("arch",                _fmt_str(m.arch, "unknown")),
        ("compiler_name",       _fmt_str(m.compiler_name, "unknown")),
        ("compiler_version",    _fmt_str(m.compiler_version, "unknown")),
        ("build_type",          _fmt_str(m.build_type, "unknown")),
        ("build_flags",         _fmt_str(m.build_flags, "unknown")),
        ("cpu_model",           _fmt_str(m.cpu_model, "unknown")),
        ("cpu_cores_logical",   _fmt_int(m.cpu_cores_logical)),
        ("cpu_cores_physical",  _fmt_int(m.cpu_cores_physical)),
    ]

    return "## Run Metadata\n\n" + _render_table(rows)


# ---------------------------------------------------------------------------
# Section: Run overview
# ---------------------------------------------------------------------------

def _report_overview(run: RunModel) -> str:
    """Render a brief overview of what is in this run."""
    rows: list[tuple[str, str]] = [
        ("summary records",    str(len(run.summary))),
        ("timeseries records", str(len(run.timeseries))),
    ]

    if run.summary:
        # Use RunModel properties — stable-sorted, deduplicated.
        statuses = sorted({r.status for r in run.summary if r.status})
        categories = sorted({r.metadata.experiment_category for r in run.summary
                              if r.metadata.experiment_category})

        rows.append(("experiments",  ", ".join(run.experiments)  or _NA))
        rows.append(("allocators",   ", ".join(run.allocators)   or _NA))
        rows.append(("workloads",    ", ".join(run.workloads)     or _NA))
        rows.append(("profiles",     ", ".join(run.profiles)      or _NA))
        rows.append(("categories",   ", ".join(categories)        or _NA))
        rows.append(("run statuses", ", ".join(statuses)          or _NA))

    return "## Run Overview\n\n" + _render_table(rows)


# ---------------------------------------------------------------------------
# Section: Summary metrics table
# ---------------------------------------------------------------------------

# Ordered column definitions: (header, record_attr, formatter)
_SUMMARY_COLUMNS: list[tuple[str, str, object]] = [
    # Identity
    ("repetition",   None,                       None),   # synthetic: row index
    ("status",       "status",                   _fmt_str),
    ("failure",      "failure_class",             _fmt_str),
    ("warmup",       "warmup_iterations",         _fmt_int),
    ("measured",     "measured_repetitions",      _fmt_int),
    # Timing
    ("phase_dur",    "phase_duration_ns",         _fmt_ns),
    ("rep_min",      "repetition_min_ns",         _fmt_ns),
    ("rep_median",   "repetition_median_ns",      _fmt_ns),
    ("rep_mean",     "repetition_mean_ns",        lambda v: _fmt_ns(int(v)) if v is not None else _NA),
    ("rep_p95",      "repetition_p95_ns",         _fmt_ns),
    ("rep_max",      "repetition_max_ns",         _fmt_ns),
    # Counters
    ("alloc_ops",    "alloc_ops_total",           _fmt_int),
    ("free_ops",     "free_ops_total",            _fmt_int),
    ("alloc_bytes",  "bytes_allocated_total",     _fmt_bytes),
    ("free_bytes",   "bytes_freed_total",         _fmt_bytes),
    ("peak_count",   "peak_live_count",           _fmt_int),
    ("peak_bytes",   "peak_live_bytes",           _fmt_bytes),
    ("final_count",  "final_live_count",          _fmt_int),
    ("final_bytes",  "final_live_bytes",          _fmt_bytes),
    # Derived
    ("ops/sec",      "ops_per_sec_mean",          lambda v: _fmt_float(v, 0) if v is not None else _NA),
    ("throughput",   "throughput_bytes_per_sec_mean", lambda v: _fmt_bytes(int(v)) if v is not None else _NA),
]


def _format_record_row(index: int, record: SummaryRecord) -> list[str]:
    """Convert one SummaryRecord into a list of cell strings."""
    cells: list[str] = []
    for header, attr, fmt in _SUMMARY_COLUMNS:
        if attr is None:
            # synthetic "repetition" column
            cells.append(str(index + 1))
        else:
            raw = getattr(record, attr, None)
            cells.append(fmt(raw))  # type: ignore[operator]
    return cells


def report_summary_table(run: RunModel) -> str:
    """
    Render a compact Markdown table of summary metrics for the given run.

    Rows are ordered by their position in run.summary (ingestion order,
    which matches repetition order from the C++ runner).  NA values are
    displayed as the literal string "NA".

    Args:
        run: An assembled RunModel.

    Returns:
        Formatted Markdown table string, or a notice if no summary data.
    """
    if not run.summary:
        return "## Summary Metrics\n\n_No summary data available._"

    headers = [col[0] for col in _SUMMARY_COLUMNS]
    rows = [_format_record_row(i, r) for i, r in enumerate(run.summary)]

    return "## Summary Metrics\n\n" + _render_multi_col_table(headers, rows)


# ---------------------------------------------------------------------------
# Public API
# ---------------------------------------------------------------------------

def report_run(run: RunModel) -> str:
    """
    Render a full compact report for a single benchmark run.

    Sections (in order):
      1. Run Metadata
      2. Run Overview
      3. Summary Metrics table

    NA values are always shown explicitly as "NA".
    Output is deterministic: section order and column/row order are fixed.

    Args:
        run: An assembled RunModel.

    Returns:
        Full report as a single string.
    """
    sections = [
        f"# Benchmark Run Report",
        "",
        report_metadata(run),
        "",
        _report_overview(run),
        "",
        report_summary_table(run),
    ]
    return "\n".join(sections)


def print_run(run: RunModel) -> None:
    """Print report_run() to stdout."""
    print(report_run(run))
