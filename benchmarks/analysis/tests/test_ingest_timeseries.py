"""
test_ingest_timeseries.py — tests for ts.v2 JSONL ingestion.

Covers:
  - Happy-path: all fields present → correct values
  - Blank lines skipped
  - Payload: absent/null → {}, dict → kept, non-dict → {} + warning
  - Malformed integer fields: present but unparseable → fallback + warning
  - Invalid JSON line → ValueError with line number
  - Non-object JSON line → ValueError
  - Missing file → FileNotFoundError
"""
from __future__ import annotations

import json
import warnings

import pytest

from analysis.ingest_timeseries import load_timeseries
from analysis.tests.conftest import make_metadata, make_ts_record, write_timeseries_jsonl


# ---------------------------------------------------------------------------
# Happy-path
# ---------------------------------------------------------------------------

class TestLoadTimeseriesHappyPath:
    def test_single_record_fields(self, tmp_path):
        meta = make_metadata(schema_version="ts.v2")
        rec = make_ts_record(meta=meta, timestamp_ns=2_000_000_000, repetition_id=3)
        path = str(tmp_path / "run.jsonl")
        write_timeseries_jsonl(path, [rec])

        loaded = load_timeseries(path)

        assert len(loaded) == 1
        r = loaded[0]
        assert r.metadata.run_id == "run-test-001"
        assert r.metadata.schema_version == "ts.v2"
        assert r.timestamp_ns == 2_000_000_000
        assert r.repetition_id == 3
        assert r.record_type == "tick"
        assert r.phase_name == "measure"

    def test_multi_record_order_preserved(self, tmp_path):
        meta = make_metadata(schema_version="ts.v2")
        recs = [make_ts_record(meta=meta, timestamp_ns=1_000_000_000 + i * 1_000_000)
                for i in range(5)]
        path = str(tmp_path / "run.jsonl")
        write_timeseries_jsonl(path, recs)

        loaded = load_timeseries(path)

        assert len(loaded) == 5
        timestamps = [r.timestamp_ns for r in loaded]
        assert timestamps == sorted(timestamps)

    def test_payload_dict_preserved(self, tmp_path):
        meta = make_metadata(schema_version="ts.v2")
        rec = make_ts_record(meta=meta, payload={"live_bytes": 8192, "alloc_count": 16})
        path = str(tmp_path / "run.jsonl")
        write_timeseries_jsonl(path, [rec])

        loaded = load_timeseries(path)

        assert loaded[0].payload == {"live_bytes": 8192, "alloc_count": 16}

    def test_blank_lines_skipped(self, tmp_path):
        meta = make_metadata(schema_version="ts.v2")
        rec = make_ts_record(meta=meta)
        path = str(tmp_path / "run_blanks.jsonl")
        # Write with blank lines between records.
        with open(path, "w", encoding="utf-8") as fh:
            obj = {
                "schema_version": "ts.v2",
                "run_id": "run-test-001",
                "experiment_name": "exp_basic",
                "experiment_category": "allocation",
                "allocator": "tlsf",
                "seed": 42,
                "run_timestamp_utc": "2025-01-01T00:00:00Z",
                "os_name": "Linux", "os_version": "5.15", "arch": "x86_64",
                "compiler_name": "clang", "compiler_version": "17.0",
                "build_type": "Release", "build_flags": "-O2",
                "cpu_model": "Intel Xeon",
                "cpu_cores_logical": 8, "cpu_cores_physical": 4,
                "status": "Valid", "failure_class": "None",
                "record_type": "tick",
                "repetition_id": 0, "timestamp_ns": 1_000_000_000,
                "event_seq_no": 0, "phase_name": "measure",
                "payload": {},
            }
            fh.write("\n")
            fh.write(json.dumps(obj) + "\n")
            fh.write("   \n")
            fh.write(json.dumps(obj) + "\n")
            fh.write("\n")

        loaded = load_timeseries(path)

        assert len(loaded) == 2


# ---------------------------------------------------------------------------
# Payload edge cases
# ---------------------------------------------------------------------------

class TestPayloadHandling:
    def _write_raw_jsonl(self, path: str, obj: dict) -> None:
        with open(path, "w", encoding="utf-8") as fh:
            fh.write(json.dumps(obj) + "\n")

    def _base_obj(self) -> dict:
        return {
            "schema_version": "ts.v2",
            "run_id": "r1", "experiment_name": "e", "experiment_category": "c",
            "allocator": "a", "seed": 1,
            "os_name": "L", "os_version": "1", "arch": "x",
            "compiler_name": "c", "compiler_version": "1",
            "build_type": "R", "build_flags": "-O2", "cpu_model": "x",
            "record_type": "tick", "repetition_id": 0,
            "timestamp_ns": 1, "event_seq_no": 0, "phase_name": "p",
        }

    def test_absent_payload_becomes_empty_dict(self, tmp_path):
        obj = self._base_obj()
        # No "payload" key at all.
        path = str(tmp_path / "no_payload.jsonl")
        self._write_raw_jsonl(path, obj)

        loaded = load_timeseries(path)
        assert loaded[0].payload == {}

    def test_null_payload_becomes_empty_dict(self, tmp_path):
        obj = self._base_obj()
        obj["payload"] = None
        path = str(tmp_path / "null_payload.jsonl")
        self._write_raw_jsonl(path, obj)

        loaded = load_timeseries(path)
        assert loaded[0].payload == {}

    def test_non_dict_payload_becomes_empty_dict_with_warning(self, tmp_path):
        obj = self._base_obj()
        obj["payload"] = "not_a_dict"
        path = str(tmp_path / "str_payload.jsonl")
        self._write_raw_jsonl(path, obj)

        with warnings.catch_warnings(record=True) as w:
            warnings.simplefilter("always")
            loaded = load_timeseries(path)

        assert loaded[0].payload == {}
        assert any("payload" in str(x.message).lower() for x in w)


# ---------------------------------------------------------------------------
# Malformed integer fields
# ---------------------------------------------------------------------------

class TestMalformedIntegerFields:
    def _base_obj(self) -> dict:
        return {
            "schema_version": "ts.v2",
            "run_id": "r1", "experiment_name": "e", "experiment_category": "c",
            "allocator": "a", "seed": 1,
            "os_name": "L", "os_version": "1", "arch": "x",
            "compiler_name": "c", "compiler_version": "1",
            "build_type": "R", "build_flags": "-O2", "cpu_model": "x",
            "record_type": "tick", "repetition_id": 0,
            "timestamp_ns": 1_000_000_000, "event_seq_no": 0, "phase_name": "p",
            "payload": {},
        }

    def test_malformed_timestamp_emits_warning(self, tmp_path):
        obj = self._base_obj()
        obj["timestamp_ns"] = "not_a_number"
        path = str(tmp_path / "bad_ts.jsonl")
        with open(path, "w", encoding="utf-8") as fh:
            fh.write(json.dumps(obj) + "\n")

        with warnings.catch_warnings(record=True) as w:
            warnings.simplefilter("always")
            loaded = load_timeseries(path)

        # Should not crash; malformed value emits a warning.
        assert len(loaded) == 1
        assert any("timestamp_ns" in str(x.message) for x in w)


# ---------------------------------------------------------------------------
# Error cases
# ---------------------------------------------------------------------------

class TestLoadTimeseriesErrors:
    def test_missing_file_raises(self, tmp_path):
        with pytest.raises(FileNotFoundError):
            load_timeseries(str(tmp_path / "nonexistent.jsonl"))

    def test_invalid_json_raises_with_line_number(self, tmp_path):
        path = str(tmp_path / "bad.jsonl")
        with open(path, "w", encoding="utf-8") as fh:
            fh.write('{"valid": true}\n')
            fh.write("NOT JSON AT ALL\n")

        with pytest.raises(ValueError, match="line 2"):
            load_timeseries(path)

    def test_json_array_line_raises(self, tmp_path):
        """A JSON array (not object) on a line must raise ValueError."""
        path = str(tmp_path / "array.jsonl")
        with open(path, "w", encoding="utf-8") as fh:
            fh.write('[1, 2, 3]\n')

        with pytest.raises(ValueError, match="expected a JSON object"):
            load_timeseries(path)

