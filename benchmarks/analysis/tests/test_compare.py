"""
test_compare.py — tests for run-to-run comparison.

Covers:
  - compare_runs: matched / unmatched record counts
  - compare_runs: NA never coerced to 0 (DiffStatus semantics)
  - compare_runs: IMPROVED / REGRESSED / SAME classification
  - compare_runs: NEUTRAL metrics produce SAME / CHANGED, never IMPROVED/REGRESSED
  - compare_runs: filter restricts both runs before matching
  - format_comparison: deterministic output, NA rendered as "NA"
  - DiffStatus enum values present in output
"""
from __future__ import annotations

import pytest

from analysis.compare import (
    compare_runs,
    format_comparison,
    DiffStatus,
    MetricDirection,
)
from analysis.models import RunMetadata, RunModel
from analysis.filters import RunFilter
from analysis.tests.conftest import make_metadata, make_summary_record


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def _make_run(run_id: str, **rec_kwargs) -> RunModel:
    meta = make_metadata(run_id=run_id)
    return RunModel(
        metadata=meta,
        summary=[make_summary_record(meta=meta, **rec_kwargs)],
        timeseries=[],
    )


def _find_metric(cmp, metric_name: str):
    """Return the MetricDiff for the given metric name from the first matched record."""
    assert cmp.matched, "No matched records"
    for md in cmp.matched[0].metrics:
        if md.metric == metric_name:
            return md
    raise KeyError(f"Metric {metric_name!r} not found")


# ---------------------------------------------------------------------------
# Matching
# ---------------------------------------------------------------------------

class TestMatching:
    def test_identical_runs_produce_matched_records(self, run_pair):
        baseline, candidate = run_pair
        cmp = compare_runs(baseline, candidate)
        assert len(cmp.matched) == 1
        assert len(cmp.unmatched_base) == 0
        assert len(cmp.unmatched_cand) == 0

    def test_extra_candidate_record_unmatched(self):
        meta = make_metadata(run_id="base")
        baseline = RunModel(
            metadata=meta,
            summary=[make_summary_record(meta=meta)],
            timeseries=[],
        )
        meta_c = make_metadata(run_id="cand", experiment_name="exp_extra")
        candidate = RunModel(
            metadata=meta_c,
            summary=[
                make_summary_record(meta=make_metadata(run_id="cand")),
                make_summary_record(meta=meta_c),
            ],
            timeseries=[],
        )
        cmp = compare_runs(baseline, candidate)
        assert len(cmp.unmatched_cand) == 1

    def test_missing_baseline_record_unmatched(self):
        meta_b = make_metadata(run_id="base", experiment_name="exp_only_base")
        meta_c = make_metadata(run_id="cand")
        baseline = RunModel(
            metadata=meta_b,
            summary=[make_summary_record(meta=meta_b)],
            timeseries=[],
        )
        candidate = RunModel(
            metadata=meta_c,
            summary=[make_summary_record(meta=meta_c)],
            timeseries=[],
        )
        cmp = compare_runs(baseline, candidate)
        assert len(cmp.unmatched_base) == 1 or len(cmp.unmatched_cand) == 1


# ---------------------------------------------------------------------------
# NA handling
# ---------------------------------------------------------------------------

class TestNaHandling:
    def test_both_na_is_both_na_status(self):
        baseline = _make_run("base", ops_per_sec_mean=None)
        candidate = _make_run("cand", ops_per_sec_mean=None)
        cmp = compare_runs(baseline, candidate)
        md = _find_metric(cmp, "ops/sec")
        assert md.status == DiffStatus.BOTH_NA

    def test_base_na_target_value(self):
        baseline = _make_run("base", ops_per_sec_mean=None)
        candidate = _make_run("cand", ops_per_sec_mean=1_000_000.0)
        cmp = compare_runs(baseline, candidate)
        md = _find_metric(cmp, "ops/sec")
        assert md.status == DiffStatus.INCOMPARABLE_BASE_NA
        assert md.abs_diff is None

    def test_base_value_target_na(self):
        baseline = _make_run("base", ops_per_sec_mean=1_000_000.0)
        candidate = _make_run("cand", ops_per_sec_mean=None)
        cmp = compare_runs(baseline, candidate)
        md = _find_metric(cmp, "ops/sec")
        assert md.status == DiffStatus.INCOMPARABLE_TARGET_NA
        assert md.rel_diff is None

    def test_na_never_equals_zero(self):
        """NA and 0 must not produce the same diff status as 0 and 0."""
        cmp_na = compare_runs(
            _make_run("base", ops_per_sec_mean=None),
            _make_run("cand", ops_per_sec_mean=None),
        )
        cmp_zero = compare_runs(
            _make_run("base", ops_per_sec_mean=0.0),
            _make_run("cand", ops_per_sec_mean=0.0),
        )
        md_na = _find_metric(cmp_na, "ops/sec")
        md_zero = _find_metric(cmp_zero, "ops/sec")
        assert md_na.status != md_zero.status, (
            "NA=NA must not have the same status as 0=0"
        )


# ---------------------------------------------------------------------------
# Direction classification
# ---------------------------------------------------------------------------

class TestDirectionClassification:
    def test_latency_decrease_is_improved(self):
        baseline = _make_run("base", repetition_median_ns=1_000_000)
        candidate = _make_run("cand", repetition_median_ns=800_000)
        cmp = compare_runs(baseline, candidate)
        md = _find_metric(cmp, "rep_median")
        assert md.status == DiffStatus.IMPROVED

    def test_latency_increase_is_regressed(self):
        baseline = _make_run("base", repetition_median_ns=800_000)
        candidate = _make_run("cand", repetition_median_ns=1_200_000)
        cmp = compare_runs(baseline, candidate)
        md = _find_metric(cmp, "rep_median")
        assert md.status == DiffStatus.REGRESSED

    def test_throughput_increase_is_improved(self):
        baseline = _make_run("base", ops_per_sec_mean=1_000_000.0)
        candidate = _make_run("cand", ops_per_sec_mean=1_500_000.0)
        cmp = compare_runs(baseline, candidate)
        md = _find_metric(cmp, "ops/sec")
        assert md.status == DiffStatus.IMPROVED

    def test_throughput_decrease_is_regressed(self):
        baseline = _make_run("base", ops_per_sec_mean=1_500_000.0)
        candidate = _make_run("cand", ops_per_sec_mean=1_000_000.0)
        cmp = compare_runs(baseline, candidate)
        md = _find_metric(cmp, "ops/sec")
        assert md.status == DiffStatus.REGRESSED

    def test_neutral_metric_never_improved_or_regressed(self):
        """alloc_ops_total is NEUTRAL — must never be IMPROVED or REGRESSED."""
        baseline = _make_run("base", alloc_ops_total=1_000)
        candidate = _make_run("cand", alloc_ops_total=2_000)
        cmp = compare_runs(baseline, candidate)
        md = _find_metric(cmp, "alloc_ops")
        assert md.status not in (DiffStatus.IMPROVED, DiffStatus.REGRESSED)
        assert md.status in (DiffStatus.SAME, DiffStatus.CHANGED)

    def test_within_threshold_is_same(self):
        """A 0.5 % change must be classified as SAME (threshold is 1 %)."""
        baseline = _make_run("base", repetition_median_ns=1_000_000)
        candidate = _make_run("cand", repetition_median_ns=1_005_000)  # +0.5%
        cmp = compare_runs(baseline, candidate)
        md = _find_metric(cmp, "rep_median")
        assert md.status == DiffStatus.SAME


# ---------------------------------------------------------------------------
# Filtering
# ---------------------------------------------------------------------------

class TestCompareFiltering:
    def test_filter_restricts_both_runs(self):
        meta_a = make_metadata(run_id="base", allocator="tlsf")
        meta_b = make_metadata(run_id="base", allocator="mimalloc",
                               experiment_name="exp_basic")
        baseline = RunModel(
            metadata=meta_a,
            summary=[
                make_summary_record(meta=meta_a),
                make_summary_record(meta=meta_b),
            ],
            timeseries=[],
        )
        meta_ca = make_metadata(run_id="cand", allocator="tlsf")
        meta_cb = make_metadata(run_id="cand", allocator="mimalloc",
                                experiment_name="exp_basic")
        candidate = RunModel(
            metadata=meta_ca,
            summary=[
                make_summary_record(meta=meta_ca),
                make_summary_record(meta=meta_cb),
            ],
            timeseries=[],
        )
        flt = RunFilter(allocators=["tlsf"])
        cmp = compare_runs(baseline, candidate, flt)

        # Only tlsf records should be matched; mimalloc pair excluded.
        for rd in cmp.matched:
            assert rd.key.allocator == "tlsf"
        assert len(cmp.matched) == 1


# ---------------------------------------------------------------------------
# format_comparison
# ---------------------------------------------------------------------------

class TestFormatComparison:
    def test_output_contains_run_ids(self, run_pair):
        baseline, candidate = run_pair
        cmp = compare_runs(baseline, candidate)
        output = format_comparison(cmp)
        assert "run-base" in output
        assert "run-cand" in output

    def test_output_is_deterministic(self, run_pair):
        baseline, candidate = run_pair
        cmp = compare_runs(baseline, candidate)
        assert format_comparison(cmp) == format_comparison(cmp)

    def test_na_rendered_as_na_string(self):
        baseline = _make_run("base", ops_per_sec_mean=None)
        candidate = _make_run("cand", ops_per_sec_mean=None)
        cmp = compare_runs(baseline, candidate)
        output = format_comparison(cmp)
        assert "NA" in output
        assert "None" not in output

    def test_status_symbols_present(self, run_pair):
        baseline, candidate = run_pair
        cmp = compare_runs(baseline, candidate)
        output = format_comparison(cmp)
        # At least one status symbol must appear in a diff table.
        symbols = {"=", "▲", "▼", "~", "?base", "?cand", "??"}
        assert any(s in output for s in symbols)

