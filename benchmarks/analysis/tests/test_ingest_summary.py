"""
test_ingest_summary.py — tests for summary.v2 CSV ingestion.

Covers:
  - Happy-path: all columns present → correct field values
  - NA preservation: empty cells → None, never 0
  - Malformed numeric: non-numeric cell → None + UserWarning
  - Unknown extra columns: silently ignored
  - Missing file: FileNotFoundError
  - Empty file (no header): ValueError
  - Multi-row: correct record count and ordering preserved
"""
from __future__ import annotations

import csv
import io
import os
import warnings

import pytest

from analysis.ingest_summary import load_summary
from analysis.tests.conftest import (
    make_metadata,
    make_summary_record,
    write_summary_csv,
    _SUMMARY_COLUMNS,
)


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def _write_csv(path: str, rows: list[dict]) -> None:
    """Write an arbitrary CSV from a list of dicts; first row defines headers."""
    if not rows:
        return
    fieldnames = list(rows[0].keys())
    with open(path, "w", newline="", encoding="utf-8") as fh:
        writer = csv.DictWriter(fh, fieldnames=fieldnames)
        writer.writeheader()
        writer.writerows(rows)


# ---------------------------------------------------------------------------
# Happy-path
# ---------------------------------------------------------------------------

class TestLoadSummaryHappyPath:
    def test_single_record_field_values(self, tmp_path):
        meta = make_metadata()
        rec = make_summary_record(meta=meta)
        path = str(tmp_path / "run.csv")
        write_summary_csv(path, [rec])

        loaded = load_summary(path)

        assert len(loaded) == 1
        r = loaded[0]
        assert r.metadata.run_id == "run-test-001"
        assert r.metadata.allocator == "tlsf"
        assert r.metadata.seed == 42
        assert r.metadata.schema_version == "summary.v2"
        assert r.status == "Valid"
        assert r.repetition_min_ns == 400_000
        assert r.ops_per_sec_mean == pytest.approx(1_960_784.0)

    def test_multi_row_count_and_order(self, tmp_path):
        meta = make_metadata()
        recs = [
            make_summary_record(meta=meta, repetition_min_ns=100_000),
            make_summary_record(meta=meta, repetition_min_ns=200_000),
            make_summary_record(meta=meta, repetition_min_ns=300_000),
        ]
        path = str(tmp_path / "run.csv")
        write_summary_csv(path, recs)

        loaded = load_summary(path)

        assert len(loaded) == 3
        assert loaded[0].repetition_min_ns == 100_000
        assert loaded[1].repetition_min_ns == 200_000
        assert loaded[2].repetition_min_ns == 300_000

    def test_env_fields_preserved(self, tmp_path):
        meta = make_metadata(cpu_model="AMD EPYC", cpu_cores_logical=32)
        rec = make_summary_record(meta=meta)
        path = str(tmp_path / "run.csv")
        write_summary_csv(path, [rec])

        loaded = load_summary(path)

        assert loaded[0].metadata.cpu_model == "AMD EPYC"
        assert loaded[0].metadata.cpu_cores_logical == 32


# ---------------------------------------------------------------------------
# NA preservation
# ---------------------------------------------------------------------------

class TestNaPreservation:
    def test_empty_metric_cells_become_none(self, tmp_path):
        """Empty cells in numeric metric columns must be None, never 0."""
        meta = make_metadata()
        rec = make_summary_record(
            meta=meta,
            ops_per_sec_mean=None,
            throughput_bytes_per_sec_mean=None,
            repetition_p95_ns=None,
            peak_live_bytes=None,
        )
        path = str(tmp_path / "run_na.csv")
        write_summary_csv(path, [rec])

        loaded = load_summary(path)
        r = loaded[0]

        assert r.ops_per_sec_mean is None, "NA must not become 0"
        assert r.throughput_bytes_per_sec_mean is None
        assert r.repetition_p95_ns is None
        assert r.peak_live_bytes is None

    def test_none_seed_preserved(self, tmp_path):
        meta = make_metadata(seed=None)
        rec = make_summary_record(meta=meta)
        path = str(tmp_path / "run_no_seed.csv")
        write_summary_csv(path, [rec])

        loaded = load_summary(path)

        assert loaded[0].metadata.seed is None

    def test_none_cpu_cores_preserved(self, tmp_path):
        meta = make_metadata(cpu_cores_logical=None, cpu_cores_physical=None)
        rec = make_summary_record(meta=meta)
        path = str(tmp_path / "run_no_cores.csv")
        write_summary_csv(path, [rec])

        loaded = load_summary(path)
        m = loaded[0].metadata

        assert m.cpu_cores_logical is None
        assert m.cpu_cores_physical is None


# ---------------------------------------------------------------------------
# Malformed numeric values
# ---------------------------------------------------------------------------

class TestMalformedValues:
    def test_malformed_int_becomes_none_with_warning(self, tmp_path):
        """A non-numeric value in an integer column → None + UserWarning."""
        meta = make_metadata()
        rec = make_summary_record(meta=meta)
        row = {
            **{c: "" for c in _SUMMARY_COLUMNS},
            "schema_version": "summary.v2",
            "run_id": "run-001",
            "experiment_name": "exp",
            "experiment_category": "alloc",
            "allocator": "tlsf",
            "seed": "42",
            "status": "Valid",
            "failure_class": "None",
            # Malformed integer in a metric column:
            "repetition_min_ns": "NOT_A_NUMBER",
        }

        path = str(tmp_path / "bad_int.csv")
        _write_csv(path, [row])

        with warnings.catch_warnings(record=True) as w:
            warnings.simplefilter("always")
            loaded = load_summary(path)

        assert loaded[0].repetition_min_ns is None
        assert any("NOT_A_NUMBER" in str(x.message) for x in w)

    def test_malformed_float_becomes_none_with_warning(self, tmp_path):
        row = {
            **{c: "" for c in _SUMMARY_COLUMNS},
            "schema_version": "summary.v2",
            "run_id": "run-001",
            "experiment_name": "exp",
            "experiment_category": "alloc",
            "allocator": "tlsf",
            "seed": "42",
            "status": "Valid",
            "failure_class": "None",
            "ops_per_sec_mean": "INVALID",
        }

        path = str(tmp_path / "bad_float.csv")
        _write_csv(path, [row])

        with warnings.catch_warnings(record=True) as w:
            warnings.simplefilter("always")
            loaded = load_summary(path)

        assert loaded[0].ops_per_sec_mean is None
        assert any("INVALID" in str(x.message) for x in w)


# ---------------------------------------------------------------------------
# Unknown extra columns
# ---------------------------------------------------------------------------

class TestUnknownColumns:
    def test_unknown_columns_silently_ignored(self, tmp_path):
        """Extra columns not in the model must be silently skipped."""
        meta = make_metadata()
        rec = make_summary_record(meta=meta)

        path = str(tmp_path / "run_extra_cols.csv")
        with open(path, "w", newline="", encoding="utf-8") as fh:
            fieldnames = _SUMMARY_COLUMNS + ["future_field_v3", "another_new_column"]
            writer = csv.DictWriter(fh, fieldnames=fieldnames)
            writer.writeheader()
            row = {c: "" for c in fieldnames}
            from analysis.tests.conftest import _rec_to_csv_row
            row.update(_rec_to_csv_row(rec))
            row["future_field_v3"] = "ignored_value"
            row["another_new_column"] = "also_ignored"
            writer.writerow(row)

        loaded = load_summary(path)

        assert len(loaded) == 1
        assert loaded[0].metadata.run_id == "run-test-001"


# ---------------------------------------------------------------------------
# Error cases
# ---------------------------------------------------------------------------

class TestLoadSummaryErrors:
    def test_missing_file_raises(self, tmp_path):
        with pytest.raises(FileNotFoundError):
            load_summary(str(tmp_path / "nonexistent.csv"))

    def test_empty_file_raises(self, tmp_path):
        """A file with no header row at all must raise ValueError."""
        path = str(tmp_path / "empty.csv")
        open(path, "w").close()  # zero-byte file

        with pytest.raises(ValueError, match="No header row"):
            load_summary(path)

