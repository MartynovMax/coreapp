"""
test_validate.py — tests for schema and content validation.

Covers:
  - validate_summary_record: supported/unsupported schema version
  - validate_summary_record: required metadata fields missing
  - validate_summary_record: NA metrics allowed (not errors)
  - validate_timeseries_record: supported/unsupported schema version
  - validate_run: empty summary and timeseries → error
  - validate_run: valid run → no errors
"""
from __future__ import annotations

import pytest

from analysis.validate import (
    validate_summary_record,
    validate_timeseries_record,
    validate_run,
    SUPPORTED_SUMMARY_VERSIONS,
    SUPPORTED_TIMESERIES_VERSIONS,
)
from analysis.models import RunMetadata, RunModel
from analysis.tests.conftest import (
    make_metadata,
    make_summary_record,
    make_ts_record,
)


# ---------------------------------------------------------------------------
# validate_summary_record
# ---------------------------------------------------------------------------

class TestValidateSummaryRecord:
    def test_valid_record_produces_no_errors(self):
        rec = make_summary_record()
        errors = validate_summary_record(rec, index=0)
        assert errors == []

    def test_unsupported_schema_version_reported(self):
        meta = make_metadata(schema_version="summary.v99")
        rec = make_summary_record(meta=meta)
        errors = validate_summary_record(rec, index=0)
        fields = [e.field for e in errors]
        assert "schema_version" in fields

    def test_empty_run_id_reported(self):
        meta = make_metadata(run_id="")
        rec = make_summary_record(meta=meta)
        errors = validate_summary_record(rec, index=0)
        assert any("run_id" in e.field for e in errors)

    def test_empty_allocator_reported(self):
        meta = make_metadata(allocator="")
        rec = make_summary_record(meta=meta)
        errors = validate_summary_record(rec, index=0)
        assert any("allocator" in e.field for e in errors)

    def test_na_metrics_are_not_errors(self):
        """None (NA) in metric fields must never be treated as a validation failure."""
        rec = make_summary_record(
            ops_per_sec_mean=None,
            throughput_bytes_per_sec_mean=None,
            repetition_p95_ns=None,
            peak_live_bytes=None,
        )
        errors = validate_summary_record(rec, index=0)
        assert errors == []

    def test_multiple_missing_fields_all_reported(self):
        meta = make_metadata(run_id="", experiment_name="", allocator="")
        rec = make_summary_record(meta=meta)
        errors = validate_summary_record(rec, index=0)
        fields = [e.field for e in errors]
        assert any("run_id" in f for f in fields)
        assert any("experiment_name" in f for f in fields)
        assert any("allocator" in f for f in fields)


# ---------------------------------------------------------------------------
# validate_timeseries_record
# ---------------------------------------------------------------------------

class TestValidateTimeseriesRecord:
    def test_valid_record_produces_no_errors(self):
        rec = make_ts_record()
        errors = validate_timeseries_record(rec, index=0)
        assert errors == []

    def test_unsupported_schema_version_reported(self):
        meta = make_metadata(schema_version="ts.v99")
        rec = make_ts_record(meta=meta)
        errors = validate_timeseries_record(rec, index=0)
        fields = [e.field for e in errors]
        assert "schema_version" in fields

    def test_empty_run_id_reported(self):
        meta = make_metadata(run_id="", schema_version="ts.v2")
        rec = make_ts_record(meta=meta)
        errors = validate_timeseries_record(rec, index=0)
        assert any("run_id" in e.field for e in errors)


# ---------------------------------------------------------------------------
# validate_run
# ---------------------------------------------------------------------------

class TestValidateRun:
    def test_valid_summary_only_run(self):
        meta = make_metadata()
        run = RunModel(
            metadata=meta,
            summary=[make_summary_record(meta=meta)],
            timeseries=[],
        )
        errors = validate_run(run)
        assert errors == []

    def test_valid_timeseries_only_run(self):
        meta = make_metadata(schema_version="ts.v2")
        run = RunModel(
            metadata=meta,
            summary=[],
            timeseries=[make_ts_record(meta=meta)],
        )
        errors = validate_run(run)
        assert errors == []

    def test_both_empty_is_an_error(self):
        run = RunModel(metadata=make_metadata(), summary=[], timeseries=[])
        errors = validate_run(run)
        assert len(errors) > 0
        assert any("empty" in e.message.lower() for e in errors)

    def test_multiple_bad_records_all_reported(self):
        bad_meta = make_metadata(run_id="", schema_version="summary.v99")
        run = RunModel(
            metadata=bad_meta,
            summary=[
                make_summary_record(meta=bad_meta),
                make_summary_record(meta=bad_meta),
            ],
            timeseries=[],
        )
        errors = validate_run(run)
        # Both records must produce errors; total count > 2.
        assert len(errors) >= 2

    def test_error_messages_are_strings(self):
        run = RunModel(metadata=make_metadata(), summary=[], timeseries=[])
        errors = validate_run(run)
        for e in errors:
            assert isinstance(str(e), str)
            assert len(str(e)) > 0



