"""
test_report.py — tests for single-run report generation.

Covers:
  - report_run: deterministic output structure (sections present)
  - report_run: NA values render as "NA", never "0" or "None"
  - report_run: metadata fields appear correctly
  - report_run: filter narrows the summary table
  - report_run: empty run after filter produces placeholder message
  - report_summary_table: column ordering is stable across calls
  - print_run: uses stdout (smoke only)
"""
from __future__ import annotations

import io
import sys

import pytest

from analysis.models import RunMetadata, RunModel
from analysis.report import report_run, report_metadata, report_summary_table, print_run
from analysis.filters import RunFilter
from analysis.tests.conftest import make_metadata, make_summary_record


# ---------------------------------------------------------------------------
# Structural checks
# ---------------------------------------------------------------------------

class TestReportStructure:
    def test_report_contains_required_sections(self, simple_run):
        output = report_run(simple_run)
        assert "# Benchmark Run Report" in output
        assert "## Run Metadata" in output
        assert "## Run Overview" in output
        assert "## Summary Metrics" in output

    def test_metadata_section_includes_run_id(self, simple_run):
        output = report_metadata(simple_run)
        assert "run-test-001" in output

    def test_metadata_section_includes_allocator(self, simple_run):
        output = report_metadata(simple_run)
        assert "tlsf" in output

    def test_metadata_section_includes_schema_version(self, simple_run):
        output = report_metadata(simple_run)
        assert "summary.v2" in output

    def test_overview_shows_experiment(self, simple_run):
        output = report_run(simple_run)
        assert "exp_basic" in output

    def test_overview_shows_allocator(self, simple_run):
        output = report_run(simple_run)
        assert "tlsf" in output


# ---------------------------------------------------------------------------
# NA rendering
# ---------------------------------------------------------------------------

class TestNaRendering:
    def test_na_values_render_as_na_string(self, run_with_na):
        output = report_run(run_with_na)
        assert "NA" in output

    def test_na_never_renders_as_zero(self, run_with_na):
        # run_with_na has ops_per_sec_mean=None and rep_p95_ns=None.
        # Those NA fields must render as "NA" in the table, never as "0".
        # (final_live_count=0 and final_live_bytes=0 are real zeros, not NA,
        # so we only check the columns we know are None.)
        table = report_summary_table(run_with_na)
        lines = table.splitlines()

        # Locate header row to find column indices for NA fields.
        header_line = next(l for l in lines if "ops/sec" in l)
        headers = [h.strip() for h in header_line.strip("|").split("|")]

        # Find the single data row.
        data_rows = [l for l in lines
                     if l.startswith("|") and "---" not in l
                     and "repetition" not in l]
        assert data_rows, "Expected at least one data row"

        for row_line in data_rows:
            cells = [c.strip() for c in row_line.strip("|").split("|")]
            row = dict(zip(headers, cells))

            # ops/sec is None → must be "NA", never "0"
            assert row.get("ops/sec") == "NA", (
                f"ops/sec should be 'NA' but got {row.get('ops/sec')!r}"
            )
            # rep_p95 is None → must be "NA"
            assert row.get("rep_p95") == "NA", (
                f"rep_p95 should be 'NA' but got {row.get('rep_p95')!r}"
            )

    def test_none_timestamp_renders_as_na(self):
        meta = make_metadata(run_timestamp_utc=None)
        run = RunModel(
            metadata=meta,
            summary=[make_summary_record(meta=meta)],
            timeseries=[],
        )
        output = report_metadata(run)
        assert "NA" in output


# ---------------------------------------------------------------------------
# Determinism
# ---------------------------------------------------------------------------

class TestDeterminism:
    def test_report_is_identical_on_repeated_calls(self, simple_run):
        out1 = report_run(simple_run)
        out2 = report_run(simple_run)
        assert out1 == out2

    def test_summary_table_column_order_is_stable(self, two_record_run):
        table1 = report_summary_table(two_record_run)
        table2 = report_summary_table(two_record_run)
        assert table1 == table2

    def test_summary_table_headers_fixed(self, simple_run):
        table = report_summary_table(simple_run)
        header_line = table.splitlines()[2]  # after "## Summary Metrics\n\n"
        # Check a few expected column names are present in the header.
        assert "repetition" in header_line
        assert "status" in header_line
        assert "rep_min" in header_line
        assert "ops/sec" in header_line


# ---------------------------------------------------------------------------
# Filtering
# ---------------------------------------------------------------------------

class TestReportFiltering:
    def test_filter_restricts_summary_table(self):
        meta_a = make_metadata(allocator="tlsf")
        meta_b = make_metadata(allocator="mimalloc")
        run = RunModel(
            metadata=meta_a,
            summary=[
                make_summary_record(meta=meta_a),
                make_summary_record(meta=meta_b),
            ],
            timeseries=[],
        )
        flt = RunFilter(allocators=["tlsf"])
        output = report_run(run, flt)

        # tlsf record present; mimalloc record excluded.
        assert "tlsf" in output
        # The summary table section has only one data row.
        table_section = output[output.index("## Summary Metrics"):]
        data_rows = [l for l in table_section.splitlines()
                     if l.startswith("|") and "---" not in l and "repetition" not in l]
        assert len(data_rows) == 1

    def test_empty_filter_includes_all_records(self, two_record_run):
        flt = RunFilter()
        output = report_run(two_record_run, flt)
        table_section = output[output.index("## Summary Metrics"):]
        data_rows = [l for l in table_section.splitlines()
                     if l.startswith("|") and "---" not in l and "repetition" not in l]
        assert len(data_rows) == 2

    def test_no_matching_records_shows_placeholder(self):
        meta = make_metadata(allocator="tlsf")
        run = RunModel(
            metadata=meta,
            summary=[make_summary_record(meta=meta)],
            timeseries=[],
        )
        flt = RunFilter(allocators=["nonexistent"])
        output = report_run(run, flt)
        assert "No summary data available" in output


# ---------------------------------------------------------------------------
# print_run smoke
# ---------------------------------------------------------------------------

class TestPrintRun:
    def test_print_run_writes_to_stdout(self, simple_run, capsys):
        print_run(simple_run)
        captured = capsys.readouterr()
        assert "# Benchmark Run Report" in captured.out
        assert captured.err == ""


