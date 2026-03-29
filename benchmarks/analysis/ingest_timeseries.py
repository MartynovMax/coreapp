"""
ingest_timeseries.py — ingestion of ts.v2 JSONL files.

Responsibility:
  Read a .jsonl file produced by the C++ benchmark runner (schema ts.v2)
  and return a list of TimeSeriesRecord instances.

  Parsing rules:
  - One JSON object per line; blank lines are skipped.
  - Fields are mapped by name, not by position.
  - Unknown top-level keys are silently ignored (forward-compatibility).
  - The payload dict is kept raw — its structure varies by record_type
    and is intentionally left for a later processing stage.
  - Missing optional fields (e.g. phase_name) become empty string / None.
  - Schema version validation is intentionally left to validate.py.
  - Malformed JSON lines raise ValueError with the line number and
    the offending content, so the caller can decide how to handle them.
"""

from __future__ import annotations

import json
import warnings
from typing import Any, Optional

from .models import RunMetadata, TimeSeriesRecord


# ---------------------------------------------------------------------------
# Internal helpers
# ---------------------------------------------------------------------------

def _get_str(obj: dict[str, Any], key: str, fallback: str = "") -> str:
    """Return a string value; use fallback for missing or non-string keys."""
    val = obj.get(key)
    if val is None:
        return fallback
    return str(val).strip() or fallback


def _get_optional_str(obj: dict[str, Any], key: str) -> Optional[str]:
    """Return a string value, or None if the key is absent or the value is empty."""
    val = obj.get(key)
    if val is None:
        return None
    stripped = str(val).strip()
    return stripped if stripped else None


def _get_int(obj: dict[str, Any], key: str, fallback: int = 0) -> int:
    """
    Return an integer value for a required-ish field.

    - Key absent / value None  → return fallback silently (field not present
      in this record variant is expected, e.g. repetition_id on some records).
    - Value present but unparseable → emit UserWarning and return fallback
      so that the caller does not silently swallow bad data.
    """
    val = obj.get(key)
    if val is None:
        return fallback
    try:
        return int(val)
    except (TypeError, ValueError):
        warnings.warn(
            f"Field {key!r}: expected integer, got {val!r} — using fallback {fallback}",
            UserWarning,
            stacklevel=3,
        )
        return fallback


def _get_optional_int(obj: dict[str, Any], key: str) -> Optional[int]:
    """
    Return an integer value, or None if the key is absent.

    - Key absent / value None  → None (field genuinely not present).
    - Value present but unparseable → emit UserWarning and return None
      so that malformed data is not silently coerced.
    """
    val = obj.get(key)
    if val is None:
        return None
    try:
        return int(val)
    except (TypeError, ValueError):
        warnings.warn(
            f"Field {key!r}: expected integer, got {val!r} — treating as None",
            UserWarning,
            stacklevel=3,
        )
        return None


# ---------------------------------------------------------------------------
# Metadata extraction
# ---------------------------------------------------------------------------

def _parse_metadata(obj: dict[str, Any]) -> RunMetadata:
    """Extract RunMetadata from a parsed JSONL record object."""
    return RunMetadata(
        schema_version=_get_str(obj, "schema_version"),
        run_id=_get_str(obj, "run_id"),
        experiment_name=_get_str(obj, "experiment_name"),
        experiment_category=_get_str(obj, "experiment_category"),
        allocator=_get_str(obj, "allocator"),
        seed=_get_optional_int(obj, "seed"),
        run_timestamp_utc=_get_optional_str(obj, "run_timestamp_utc"),
        # Env strings: absent or empty → "unknown" (matches C++ fallback guarantee).
        os_name=_get_str(obj, "os_name", "unknown"),
        os_version=_get_str(obj, "os_version", "unknown"),
        arch=_get_str(obj, "arch", "unknown"),
        compiler_name=_get_str(obj, "compiler_name", "unknown"),
        compiler_version=_get_str(obj, "compiler_version", "unknown"),
        build_type=_get_str(obj, "build_type", "unknown"),
        build_flags=_get_str(obj, "build_flags", "unknown"),
        cpu_model=_get_str(obj, "cpu_model", "unknown"),
        # CPU core counts: Optional[int], keep None if absent.
        cpu_cores_logical=_get_optional_int(obj, "cpu_cores_logical"),
        cpu_cores_physical=_get_optional_int(obj, "cpu_cores_physical"),
    )


# ---------------------------------------------------------------------------
# Record extraction
# ---------------------------------------------------------------------------

def _parse_record(obj: dict[str, Any]) -> TimeSeriesRecord:
    """Map one parsed JSON object to a TimeSeriesRecord."""
    # Payload handling is explicit:
    #   - absent or null  → empty dict (record has no payload data)
    #   - dict            → keep as-is (normal case)
    #   - any other type  → warn and use empty dict (schema violation)
    raw_payload = obj.get("payload")
    if raw_payload is None:
        payload: dict = {}
    elif isinstance(raw_payload, dict):
        payload = raw_payload
    else:
        warnings.warn(
            f"Field 'payload': expected a JSON object, "
            f"got {type(raw_payload).__name__!r} — using empty dict",
            UserWarning,
            stacklevel=2,
        )
        payload = {}

    return TimeSeriesRecord(
        metadata=_parse_metadata(obj),

        # Run validity — written by C++ in every record alongside metadata.
        status=_get_optional_str(obj, "status"),
        failure_class=_get_optional_str(obj, "failure_class"),

        record_type=_get_str(obj, "record_type"),
        repetition_id=_get_int(obj, "repetition_id"),
        timestamp_ns=_get_int(obj, "timestamp_ns"),
        event_seq_no=_get_int(obj, "event_seq_no"),

        # Optional event-level fields (absent on some record types).
        phase_name=_get_str(obj, "phase_name"),
        event_experiment_name=_get_optional_str(obj, "event_experiment_name"),

        payload=payload,
    )


# ---------------------------------------------------------------------------
# Public API
# ---------------------------------------------------------------------------

def load_timeseries(path: str) -> list[TimeSeriesRecord]:
    """
    Load a time-series JSONL file and return one TimeSeriesRecord per line.

    Parsing rules:
    - Blank lines are skipped.
    - Each non-blank line must be a valid JSON object; if not, a ValueError
      is raised immediately with the line number and offending text.
    - Unknown top-level keys are ignored (forward-compatible with schema
      additions).
    - Schema version checking is delegated to validate.py.

    Args:
        path: Path to a .jsonl file produced with ``--format=jsonl`` or
              ``--format=all`` (schema ``ts.v2``).

    Returns:
        List of TimeSeriesRecord instances, one per non-blank line.

    Raises:
        FileNotFoundError: If ``path`` does not exist.
        ValueError: If any non-blank line is not valid JSON, or if a
                    successfully parsed line is not a JSON object.
    """
    records: list[TimeSeriesRecord] = []

    with open(path, encoding="utf-8") as fh:
        for lineno, raw_line in enumerate(fh, start=1):
            line = raw_line.strip()
            if not line:
                continue  # skip blank / whitespace-only lines

            try:
                obj = json.loads(line)
            except json.JSONDecodeError as exc:
                raise ValueError(
                    f"{path!r} line {lineno}: invalid JSON — {exc}"
                ) from exc

            if not isinstance(obj, dict):
                raise ValueError(
                    f"{path!r} line {lineno}: expected a JSON object, "
                    f"got {type(obj).__name__}"
                )

            records.append(_parse_record(obj))

    return records
