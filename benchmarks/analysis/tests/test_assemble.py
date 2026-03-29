"""
test_assemble.py — tests for RunModel assembly.

Covers:
  - assemble_run: happy path from summary + timeseries
  - assemble_run: summary only, timeseries only
  - assemble_run: both empty → ValueError
  - assemble_run: stream identity mismatch → ValueError
  - assemble_run_from_files: loads and assembles correctly
  - assemble_run_from_files: missing file → warning (not error) when optional
  - assemble_run_from_files: validation=True propagates errors
"""
from __future__ import annotations

import warnings

import pytest

from analysis.assemble import assemble_run, assemble_run_from_files
from analysis.models import RunMetadata, RunModel
from analysis.tests.conftest import (
    make_metadata,
    make_summary_record,
    make_ts_record,
    write_summary_csv,
    write_timeseries_jsonl,
)


# ---------------------------------------------------------------------------
# assemble_run (in-memory)
# ---------------------------------------------------------------------------

class TestAssembleRun:
    def test_summary_only(self):
        meta = make_metadata()
        recs = [make_summary_record(meta=meta)]
        run = assemble_run(recs, [])
        assert len(run.summary) == 1
        assert run.timeseries == []
        assert run.metadata.run_id == "run-test-001"

    def test_timeseries_only(self):
        meta = make_metadata(schema_version="ts.v2")
        recs = [make_ts_record(meta=meta)]
        run = assemble_run([], recs)
        assert run.summary == []
        assert len(run.timeseries) == 1

    def test_both_streams(self):
        meta_s = make_metadata(schema_version="summary.v2")
        meta_t = make_metadata(schema_version="ts.v2")
        srecs = [make_summary_record(meta=meta_s)]
        trecs = [make_ts_record(meta=meta_t)]
        run = assemble_run(srecs, trecs)
        assert len(run.summary) == 1
        assert len(run.timeseries) == 1

    def test_both_empty_raises(self):
        with pytest.raises(ValueError, match="both summary_records and timeseries_records are empty"):
            assemble_run([], [])

    def test_stream_mismatch_raises(self):
        meta_s = make_metadata(run_id="run-A")
        meta_t = make_metadata(run_id="run-B", schema_version="ts.v2")
        with pytest.raises(ValueError, match="run_id"):
            assemble_run(
                [make_summary_record(meta=meta_s)],
                [make_ts_record(meta=meta_t)],
            )

    def test_explicit_metadata_overrides(self):
        meta = make_metadata()
        override = RunMetadata(run_id="override-id", allocator="custom")
        run = assemble_run([make_summary_record(meta=meta)], [], metadata=override)
        assert run.metadata.run_id == "override-id"
        assert run.metadata.allocator == "custom"

    def test_summary_list_is_copied(self):
        meta = make_metadata()
        recs = [make_summary_record(meta=meta)]
        run = assemble_run(recs, [])
        recs.append(make_summary_record(meta=meta))
        assert len(run.summary) == 1  # original not mutated


# ---------------------------------------------------------------------------
# assemble_run_from_files
# ---------------------------------------------------------------------------

class TestAssembleRunFromFiles:
    def test_csv_only(self, tmp_path):
        meta = make_metadata()
        rec = make_summary_record(meta=meta)
        base = str(tmp_path / "run1")
        write_summary_csv(base + ".csv", [rec])

        with warnings.catch_warnings(record=True):
            warnings.simplefilter("always")
            run = assemble_run_from_files(base, validate=False)

        assert len(run.summary) == 1
        assert run.metadata.run_id == "run-test-001"

    def test_jsonl_only(self, tmp_path):
        meta = make_metadata(schema_version="ts.v2")
        rec = make_ts_record(meta=meta)
        base = str(tmp_path / "run_ts")
        write_timeseries_jsonl(base + ".jsonl", [rec])

        with warnings.catch_warnings(record=True):
            warnings.simplefilter("always")
            run = assemble_run_from_files(base, validate=False)

        assert len(run.timeseries) == 1

    def test_both_files(self, tmp_path):
        meta_s = make_metadata(schema_version="summary.v2")
        meta_t = make_metadata(schema_version="ts.v2")
        base = str(tmp_path / "run_full")
        write_summary_csv(base + ".csv", [make_summary_record(meta=meta_s)])
        write_timeseries_jsonl(base + ".jsonl", [make_ts_record(meta=meta_t)])

        run = assemble_run_from_files(base, validate=False)

        assert len(run.summary) == 1
        assert len(run.timeseries) == 1

    def test_missing_optional_csv_emits_warning(self, tmp_path):
        meta = make_metadata(schema_version="ts.v2")
        base = str(tmp_path / "ts_only")
        write_timeseries_jsonl(base + ".jsonl", [make_ts_record(meta=meta)])

        with warnings.catch_warnings(record=True) as w:
            warnings.simplefilter("always")
            run = assemble_run_from_files(base, validate=False)

        assert run.summary == []
        assert any("summary" in str(x.message).lower() for x in w)

    def test_both_files_missing_raises(self, tmp_path):
        with pytest.raises((ValueError, FileNotFoundError)):
            assemble_run_from_files(str(tmp_path / "nonexistent"), validate=False)

    def test_required_csv_missing_raises(self, tmp_path):
        base = str(tmp_path / "missing")
        with pytest.raises(FileNotFoundError):
            assemble_run_from_files(base, require_summary=True, validate=False)

    def test_validation_propagates_errors(self, tmp_path):
        # Write a CSV with an invalid schema version.
        meta = make_metadata(schema_version="summary.v99")
        rec = make_summary_record(meta=meta)
        base = str(tmp_path / "bad_schema")
        write_summary_csv(base + ".csv", [rec])

        with warnings.catch_warnings(record=True):
            warnings.simplefilter("always")
            with pytest.raises(ValueError, match="Validation failed"):
                assemble_run_from_files(base, validate=True)

