"""
compare.py — run-to-run comparison of benchmark results.

Responsibility:
  Compare two RunModel instances and produce a structured diff report.

  Matching is performed on (experiment_name, allocator, repetition index).
  Records that exist in only one run are marked as unmatched.

  NA handling rules (strict):
  - NA is never coerced to zero.
  - base=value,  target=NA   → status INCOMPARABLE_TARGET_NA
  - base=NA,     target=value → status INCOMPARABLE_BASE_NA
  - base=NA,     target=NA   → status BOTH_NA
  - base=value,  target=value → numeric diff; classified by MetricDirection.

  Metric direction (MetricDirection enum):
  - LOWER_IS_BETTER  — latency / duration; decrease → IMPROVED, increase → REGRESSED.
  - HIGHER_IS_BETTER — throughput / rate;  increase → IMPROVED, decrease → REGRESSED.
  - NEUTRAL          — workload-characterisation counters (alloc volume, live bytes, etc.).
                       These track what the workload did, not how efficiently.
                       Status is SAME or CHANGED only — never IMPROVED/REGRESSED —
                       so the report does not mislead on directionless metrics.

  A 1 % relative threshold separates SAME from IMPROVED / REGRESSED / CHANGED.

Public API:
  compare_runs(baseline, candidate)  → RunComparison
  format_comparison(cmp)             → str  (Markdown report)
  print_comparison(cmp)              → None (prints to stdout)
"""

from __future__ import annotations

import math
from dataclasses import dataclass, field
from enum import Enum
from typing import Optional

from .models import RunModel, SummaryRecord
from .filters import RunFilter, apply_filter


# ---------------------------------------------------------------------------
# Direction and status enums
# ---------------------------------------------------------------------------

class MetricDirection(str, Enum):
    """
    Semantic direction for a metric — controls how numeric change is classified.

    LOWER_IS_BETTER:
        A decrease in value is an improvement (e.g. latency, duration).
        Status: IMPROVED / SAME / REGRESSED.

    HIGHER_IS_BETTER:
        An increase in value is an improvement (e.g. ops/sec, throughput).
        Status: IMPROVED / SAME / REGRESSED.

    NEUTRAL:
        No meaningful direction can be assigned without workload context
        (e.g. total allocation count, peak live bytes — these track what the
        workload *did*, not how fast or efficiently it ran).
        Status: CHANGED / SAME only.  IMPROVED / REGRESSED are never emitted
        so the report does not mislead the reader into thinking a difference
        is inherently good or bad.
    """

    LOWER_IS_BETTER  = "lower_is_better"
    HIGHER_IS_BETTER = "higher_is_better"
    NEUTRAL          = "neutral"


class DiffStatus(str, Enum):
    """Classification of a single metric comparison result."""

    SAME           = "same"
    IMPROVED       = "improved"
    REGRESSED      = "regressed"
    CHANGED        = "changed"          # numeric difference, direction is NEUTRAL
    INCOMPARABLE_BASE_NA   = "base_na"
    INCOMPARABLE_TARGET_NA = "target_na"
    BOTH_NA        = "both_na"


# ---------------------------------------------------------------------------
# Per-metric result
# ---------------------------------------------------------------------------

@dataclass
class MetricDiff:
    """
    Comparison result for a single numeric metric across two runs.

    Attributes:
        metric:     Metric name (matches SummaryRecord field name).
        base:       Value from the baseline run; None means NA.
        target:     Value from the candidate run; None means NA.
        abs_diff:   target - base, or None when not computable.
        rel_diff:   abs_diff / abs(base), or None when not computable.
        status:     Classification of the difference.
                    NEUTRAL metrics only produce SAME / CHANGED, never
                    IMPROVED / REGRESSED.
    """

    metric: str
    base: Optional[float]
    target: Optional[float]
    abs_diff: Optional[float]
    rel_diff: Optional[float]
    status: DiffStatus


# ---------------------------------------------------------------------------
# Per-record pair result
# ---------------------------------------------------------------------------

@dataclass
class RecordKey:
    """Identity key used to match a summary record across two runs."""

    experiment_name: str
    allocator: str
    repetition_index: int   # 0-based position within the run

    def __str__(self) -> str:
        return f"{self.experiment_name} / {self.allocator} / rep {self.repetition_index}"


@dataclass
class RecordDiff:
    """
    Comparison result for one matched pair of SummaryRecord rows.

    Attributes:
        key:     Identity used to match the records.
        metrics: Ordered list of per-metric diffs for the standard metric set.
    """

    key: RecordKey
    metrics: list[MetricDiff] = field(default_factory=list)


# ---------------------------------------------------------------------------
# Top-level comparison result
# ---------------------------------------------------------------------------

@dataclass
class RunComparison:
    """
    Full comparison between a baseline run and a candidate run.

    Attributes:
        baseline_run_id:  run_id from the baseline RunModel.
        candidate_run_id: run_id from the candidate RunModel.
        matched:          RecordDiff for each matched pair.
        unmatched_base:   Keys present only in the baseline.
        unmatched_cand:   Keys present only in the candidate.
    """

    baseline_run_id: str
    candidate_run_id: str
    matched: list[RecordDiff] = field(default_factory=list)
    unmatched_base: list[RecordKey] = field(default_factory=list)
    unmatched_cand: list[RecordKey] = field(default_factory=list)


# ---------------------------------------------------------------------------
# Metric catalogue
#
# Tuple: (field_name, display_name, direction)
#
# Direction semantics:
#   LOWER_IS_BETTER  — latency / duration metrics; a decrease is good.
#   HIGHER_IS_BETTER — throughput / rate metrics; an increase is good.
#   NEUTRAL          — workload-characterisation counters that measure *what
#                      happened*, not *how well*.  These track allocation
#                      volume, live object counts, etc.  Whether a change is
#                      good or bad depends entirely on the workload intent and
#                      cannot be decided by the tool.  They are reported as
#                      CHANGED (not IMPROVED/REGRESSED) to avoid misleading
#                      the reader.
# ---------------------------------------------------------------------------

_LIB = MetricDirection.LOWER_IS_BETTER
_HIB = MetricDirection.HIGHER_IS_BETTER
_NEU = MetricDirection.NEUTRAL

_NUMERIC_METRICS: list[tuple[str, str, MetricDirection]] = [
    # Timing — lower latency is better
    ("phase_duration_ns",             "phase_dur",    _LIB),
    ("repetition_min_ns",             "rep_min",      _LIB),
    ("repetition_median_ns",          "rep_median",   _LIB),
    ("repetition_mean_ns",            "rep_mean",     _LIB),
    ("repetition_p95_ns",             "rep_p95",      _LIB),
    ("repetition_max_ns",             "rep_max",      _LIB),
    # Workload-characterisation counters — NEUTRAL
    # These record how much work the workload performed, not how efficiently.
    # A change in alloc count may be expected, coincidental, or intentional —
    # the tool cannot determine which without workload context.
    ("alloc_ops_total",               "alloc_ops",    _NEU),
    ("free_ops_total",                "free_ops",     _NEU),
    ("bytes_allocated_total",         "alloc_bytes",  _NEU),
    ("bytes_freed_total",             "free_bytes",   _NEU),
    ("peak_live_count",               "peak_count",   _NEU),
    ("peak_live_bytes",               "peak_bytes",   _NEU),
    ("final_live_count",              "final_count",  _NEU),
    ("final_live_bytes",              "final_bytes",  _NEU),
    # Derived throughput — higher is better
    ("ops_per_sec_mean",              "ops/sec",      _HIB),
    ("throughput_bytes_per_sec_mean", "throughput",   _HIB),
]

# Threshold below which relative change is classified as SAME (1 %)
_SAME_THRESHOLD: float = 0.01


# ---------------------------------------------------------------------------
# Numeric helpers
# ---------------------------------------------------------------------------

def _to_float(value: Optional[int | float]) -> Optional[float]:
    """Coerce an int/float field value to float, preserving None."""
    if value is None:
        return None
    return float(value)


def _compute_diff(
    base: Optional[float],
    target: Optional[float],
    direction: MetricDirection,
) -> MetricDiff:
    """
    Compute a MetricDiff for a single (base, target) pair.

    NA is never coerced to zero.  All four NA combinations are handled
    explicitly before any arithmetic is attempted.

    For NEUTRAL metrics abs_diff and rel_diff are still computed so the
    reader can see the magnitude of change, but the status is always
    SAME or CHANGED — never IMPROVED or REGRESSED.
    """
    if base is None and target is None:
        return MetricDiff("", base, target, None, None, DiffStatus.BOTH_NA)

    if base is None:
        return MetricDiff("", base, target, None, None, DiffStatus.INCOMPARABLE_BASE_NA)

    if target is None:
        return MetricDiff("", base, target, None, None, DiffStatus.INCOMPARABLE_TARGET_NA)

    abs_diff = target - base

    # Relative diff: undefined when base is exactly zero.
    rel_diff: Optional[float] = None if base == 0.0 else abs_diff / abs(base)

    # --- Neutral direction: only SAME / CHANGED ---
    if direction is MetricDirection.NEUTRAL:
        if rel_diff is None:
            status = DiffStatus.SAME if abs_diff == 0.0 else DiffStatus.CHANGED
        else:
            status = DiffStatus.SAME if abs(rel_diff) <= _SAME_THRESHOLD else DiffStatus.CHANGED
        return MetricDiff("", base, target, abs_diff, rel_diff, status)

    # --- Directional metrics: SAME / IMPROVED / REGRESSED ---
    lower_is_better = direction is MetricDirection.LOWER_IS_BETTER

    if rel_diff is None:
        # base is zero
        if abs_diff == 0.0:
            status = DiffStatus.SAME
        elif lower_is_better:
            status = DiffStatus.IMPROVED if abs_diff < 0 else DiffStatus.REGRESSED
        else:
            status = DiffStatus.IMPROVED if abs_diff > 0 else DiffStatus.REGRESSED
    elif abs(rel_diff) <= _SAME_THRESHOLD:
        status = DiffStatus.SAME
    elif lower_is_better:
        status = DiffStatus.IMPROVED if rel_diff < 0 else DiffStatus.REGRESSED
    else:
        status = DiffStatus.IMPROVED if rel_diff > 0 else DiffStatus.REGRESSED

    return MetricDiff("", base, target, abs_diff, rel_diff, status)


def _diff_record_pair(
    base_rec: SummaryRecord,
    cand_rec: SummaryRecord,
) -> list[MetricDiff]:
    """Build the full list of MetricDiff for one matched record pair."""
    diffs: list[MetricDiff] = []
    for field_name, display_name, direction in _NUMERIC_METRICS:
        base_val = _to_float(getattr(base_rec, field_name, None))
        cand_val = _to_float(getattr(cand_rec, field_name, None))
        diff = _compute_diff(base_val, cand_val, direction)
        diff.metric = display_name
        diffs.append(diff)
    return diffs


# ---------------------------------------------------------------------------
# Record indexing
# ---------------------------------------------------------------------------

def _index_records(
    records: list[SummaryRecord],
) -> dict[tuple[str, str, int], SummaryRecord]:
    """
    Build a lookup dict keyed by (experiment_name, allocator, repetition_index).

    Repetition index is the 0-based position of the record within the list —
    it reflects the order produced by the C++ runner.
    """
    index: dict[tuple[str, str, int], SummaryRecord] = {}
    counters: dict[tuple[str, str], int] = {}

    for record in records:
        exp = record.metadata.experiment_name
        alloc = record.metadata.allocator
        group = (exp, alloc)
        rep_idx = counters.get(group, 0)
        counters[group] = rep_idx + 1
        index[(exp, alloc, rep_idx)] = record

    return index


# ---------------------------------------------------------------------------
# Public API
# ---------------------------------------------------------------------------

def compare_runs(
    baseline: RunModel,
    candidate: RunModel,
    flt: Optional[RunFilter] = None,
) -> RunComparison:
    """
    Compare two RunModel instances and return a RunComparison.

    If *flt* is provided, the same filter is applied to both runs before
    matching so that the comparison scope is restricted to the records that
    satisfy the filter.  Filtering is done on both sides independently to
    preserve correct unmatched tracking.

    Matching strategy:
    - Records are matched by (experiment_name, allocator, repetition_index).
    - Repetition index is the 0-based position within records sharing the
      same (experiment_name, allocator) pair in the respective run.
    - Records with no counterpart in the other run are collected as
      unmatched_base / unmatched_cand.

    NA handling:
    - NA (None) is never treated as zero.
    - Each MetricDiff carries an explicit DiffStatus for all NA cases.

    Args:
        baseline:  The reference RunModel.
        candidate: The RunModel being evaluated against the baseline.
        flt:       Optional RunFilter; pass None to compare all records.

    Returns:
        A RunComparison with matched diffs and unmatched key lists.
    """
    if flt is not None:
        baseline  = apply_filter(baseline,  flt)
        candidate = apply_filter(candidate, flt)

    cmp = RunComparison(
        baseline_run_id=baseline.metadata.run_id or "(unknown)",
        candidate_run_id=candidate.metadata.run_id or "(unknown)",
    )

    base_index = _index_records(baseline.summary)
    cand_index = _index_records(candidate.summary)

    all_keys = sorted(set(base_index) | set(cand_index))

    for key_tuple in all_keys:
        exp, alloc, rep_idx = key_tuple
        rec_key = RecordKey(exp, alloc, rep_idx)

        in_base = key_tuple in base_index
        in_cand = key_tuple in cand_index

        if in_base and in_cand:
            metric_diffs = _diff_record_pair(base_index[key_tuple], cand_index[key_tuple])
            cmp.matched.append(RecordDiff(key=rec_key, metrics=metric_diffs))
        elif in_base:
            cmp.unmatched_base.append(rec_key)
        else:
            cmp.unmatched_cand.append(rec_key)

    return cmp


# ---------------------------------------------------------------------------
# Formatting helpers
# ---------------------------------------------------------------------------

_NA_STR = "NA"
_STATUS_SYMBOL: dict[DiffStatus, str] = {
    DiffStatus.SAME:                   "=",
    DiffStatus.IMPROVED:               "▲",
    DiffStatus.REGRESSED:              "▼",
    DiffStatus.CHANGED:                "~",
    DiffStatus.INCOMPARABLE_BASE_NA:   "?base",
    DiffStatus.INCOMPARABLE_TARGET_NA: "?cand",
    DiffStatus.BOTH_NA:                "??",
}


def _fmt_val(v: Optional[float]) -> str:
    if v is None:
        return _NA_STR
    if v == math.floor(v) and abs(v) < 1e15:
        return str(int(v))
    return f"{v:.4g}"


def _fmt_rel(v: Optional[float]) -> str:
    if v is None:
        return _NA_STR
    return f"{v * 100:+.2f}%"


def _render_table(headers: list[str], rows: list[list[str]]) -> str:
    """Simple fixed-width Markdown table renderer (no external deps)."""
    n = len(headers)
    widths = [len(h) for h in headers]
    for row in rows:
        for i, cell in enumerate(row):
            widths[i] = max(widths[i], len(cell))

    def fmt_row(cells: list[str]) -> str:
        return "| " + " | ".join(f"{c:<{widths[i]}}" for i, c in enumerate(cells)) + " |"

    sep = "| " + " | ".join("-" * w for w in widths) + " |"
    lines = [fmt_row(headers), sep] + [fmt_row(r) for r in rows]
    return "\n".join(lines)


def _render_record_diff(rd: RecordDiff) -> str:
    """Render one RecordDiff as a Markdown subsection with a metrics table."""
    headers = ["metric", "base", "target", "abs_diff", "rel_diff", "status"]
    rows: list[list[str]] = []
    for md in rd.metrics:
        rows.append([
            md.metric,
            _fmt_val(md.base),
            _fmt_val(md.target),
            _fmt_val(md.abs_diff),
            _fmt_rel(md.rel_diff),
            _STATUS_SYMBOL[md.status],
        ])
    return f"### {rd.key}\n\n" + _render_table(headers, rows)


def format_comparison(cmp: RunComparison) -> str:
    """
    Render a RunComparison as a human-readable Markdown report.

    Sections (in order):
      1. Comparison header (run IDs)
      2. Summary counts (matched / unmatched)
      3. Per-record diff tables
      4. Unmatched records (if any)

    NA values are always displayed as "NA" and never as zero.
    Output is deterministic: record order follows sorted matching keys.

    Args:
        cmp: Output of compare_runs().

    Returns:
        Full report as a single string.
    """
    lines: list[str] = []

    # --- header ---
    lines.append("# Benchmark Run Comparison")
    lines.append("")
    lines.append(f"**Baseline:**  `{cmp.baseline_run_id}`")
    lines.append(f"**Candidate:** `{cmp.candidate_run_id}`")
    lines.append("")

    # --- summary counts ---
    lines.append("## Summary")
    lines.append("")
    count_rows = [
        ["matched records",            str(len(cmp.matched))],
        ["unmatched (baseline only)",  str(len(cmp.unmatched_base))],
        ["unmatched (candidate only)", str(len(cmp.unmatched_cand))],
    ]
    lines.append(_render_table(["item", "count"], count_rows))
    lines.append("")

    # --- per-record diffs ---
    if cmp.matched:
        lines.append("## Record Diffs")
        lines.append("")
        for rd in cmp.matched:
            lines.append(_render_record_diff(rd))
            lines.append("")
    else:
        lines.append("## Record Diffs")
        lines.append("")
        lines.append("_No matched records._")
        lines.append("")

    # --- unmatched ---
    if cmp.unmatched_base or cmp.unmatched_cand:
        lines.append("## Unmatched Records")
        lines.append("")
        if cmp.unmatched_base:
            lines.append("**Present in baseline only:**")
            for k in cmp.unmatched_base:
                lines.append(f"- {k}")
            lines.append("")
        if cmp.unmatched_cand:
            lines.append("**Present in candidate only:**")
            for k in cmp.unmatched_cand:
                lines.append(f"- {k}")
            lines.append("")

    return "\n".join(lines)


def print_comparison(cmp: RunComparison) -> None:
    """Print format_comparison() to stdout."""
    print(format_comparison(cmp))
