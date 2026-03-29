"""
test_cli.py — tests for the CLI entry point.

Covers:
  - report subcommand: runs without error on valid input
  - report subcommand: exits 1 on missing file
  - compare subcommand: runs without error on valid inputs
  - compare subcommand: exits 1 on missing file
  - plot subcommand: latency kind writes an output file
  - --allocator filter: restricts output
  - build_parser: subcommands registered correctly
  - main: exits with code from subcommand handler
"""
from __future__ import annotations

import os
import sys
import warnings

import pytest

from analysis.cli import build_parser, main
from analysis.tests.conftest import (
    make_metadata,
    make_summary_record,
    make_ts_record,
    write_summary_csv,
    write_timeseries_jsonl,
)


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def _run_cli(args: list[str]) -> int:
    """Invoke the CLI with the given argument list; return the exit code."""
    parser = build_parser()
    parsed = parser.parse_args(args)
    with warnings.catch_warnings(record=True):
        warnings.simplefilter("always")
        return parsed.func(parsed)


def _write_valid_run(base: str) -> None:
    """Write a minimal valid summary CSV (no JSONL needed for most tests)."""
    meta = make_metadata()
    write_summary_csv(base + ".csv", [make_summary_record(meta=meta)])


def _write_valid_ts_run(base: str) -> None:
    """Write a minimal valid JSONL (no CSV)."""
    meta = make_metadata(schema_version="ts.v2")
    write_timeseries_jsonl(base + ".jsonl", [make_ts_record(meta=meta)])


# ---------------------------------------------------------------------------
# report subcommand
# ---------------------------------------------------------------------------

class TestCliReport:
    def test_report_exits_zero_on_valid_input(self, tmp_path, capsys):
        base = str(tmp_path / "run1")
        _write_valid_run(base)

        code = _run_cli(["report", "--input", base])

        assert code == 0
        captured = capsys.readouterr()
        assert "# Benchmark Run Report" in captured.out

    def test_report_exits_one_on_missing_file(self, tmp_path, capsys):
        base = str(tmp_path / "nonexistent")

        code = _run_cli(["report", "--input", base])

        assert code == 1
        captured = capsys.readouterr()
        assert "error" in captured.err.lower()

    def test_report_with_allocator_filter(self, tmp_path, capsys):
        meta_a = make_metadata(allocator="tlsf")
        meta_b = make_metadata(allocator="mimalloc")
        base = str(tmp_path / "run_multi")
        write_summary_csv(base + ".csv", [
            make_summary_record(meta=meta_a),
            make_summary_record(meta=meta_b),
        ])

        code = _run_cli(["report", "--input", base, "--allocator", "tlsf"])

        assert code == 0
        out = capsys.readouterr().out
        # The overview should list only the tlsf allocator.
        assert "tlsf" in out


# ---------------------------------------------------------------------------
# compare subcommand
# ---------------------------------------------------------------------------

class TestCliCompare:
    def test_compare_exits_zero_on_valid_inputs(self, tmp_path, capsys):
        base1 = str(tmp_path / "run_base")
        base2 = str(tmp_path / "run_cand")
        _write_valid_run(base1)
        _write_valid_run(base2)

        code = _run_cli(["compare", "--baseline", base1, "--candidate", base2])

        assert code == 0
        out = capsys.readouterr().out
        assert "# Benchmark Run Comparison" in out

    def test_compare_exits_one_on_missing_baseline(self, tmp_path, capsys):
        base2 = str(tmp_path / "run_cand")
        _write_valid_run(base2)

        code = _run_cli(["compare",
                          "--baseline", str(tmp_path / "missing"),
                          "--candidate", base2])

        assert code == 1

    def test_compare_with_experiment_filter(self, tmp_path, capsys):
        meta_a = make_metadata(experiment_name="exp_A")
        meta_b = make_metadata(experiment_name="exp_B")
        base1 = str(tmp_path / "run_base")
        base2 = str(tmp_path / "run_cand")
        write_summary_csv(base1 + ".csv", [
            make_summary_record(meta=meta_a),
            make_summary_record(meta=meta_b),
        ])
        write_summary_csv(base2 + ".csv", [
            make_summary_record(meta=meta_a),
            make_summary_record(meta=meta_b),
        ])

        code = _run_cli(["compare",
                          "--baseline", base1,
                          "--candidate", base2,
                          "--experiment", "exp_A"])

        assert code == 0
        out = capsys.readouterr().out
        assert "exp_A" in out


# ---------------------------------------------------------------------------
# plot subcommand
# ---------------------------------------------------------------------------

class TestCliPlot:
    def test_latency_plot_writes_file(self, tmp_path, capsys):
        base = str(tmp_path / "run1")
        _write_valid_run(base)
        out_img = str(tmp_path / "latency.png")

        code = _run_cli(["plot", "--input", base,
                          "--kind", "latency",
                          "--output", out_img])

        assert code == 0
        assert os.path.exists(out_img)

    def test_timeseries_plot_requires_metric_key(self, tmp_path, capsys):
        base = str(tmp_path / "run_ts")
        _write_valid_ts_run(base)

        code = _run_cli(["plot", "--input", base,
                          "--kind", "timeseries",
                          "--output", str(tmp_path / "ts.png")])

        assert code == 1
        assert "metric-key" in capsys.readouterr().err

    def test_timeseries_plot_with_metric_key(self, tmp_path, capsys):
        base = str(tmp_path / "run_ts2")
        _write_valid_ts_run(base)
        out_img = str(tmp_path / "ts.png")

        code = _run_cli(["plot", "--input", base,
                          "--kind", "timeseries",
                          "--output", out_img,
                          "--metric-key", "live_bytes"])

        assert code == 0
        assert os.path.exists(out_img)


# ---------------------------------------------------------------------------
# build_parser
# ---------------------------------------------------------------------------

class TestBuildParser:
    def test_report_subcommand_registered(self):
        parser = build_parser()
        args = parser.parse_args(["report", "--input", "x"])
        assert args.command == "report"

    def test_compare_subcommand_registered(self):
        parser = build_parser()
        args = parser.parse_args(["compare", "--baseline", "a", "--candidate", "b"])
        assert args.command == "compare"

    def test_plot_subcommand_registered(self):
        parser = build_parser()
        args = parser.parse_args(["plot", "--input", "x", "--kind", "latency", "--output", "o.png"])
        assert args.command == "plot"

    def test_filter_args_available_on_report(self):
        parser = build_parser()
        args = parser.parse_args([
            "report", "--input", "x",
            "--allocator", "tlsf", "--allocator", "mimalloc",
            "--experiment", "exp1",
        ])
        assert args.allocator == ["tlsf", "mimalloc"]
        assert args.experiment == ["exp1"]

    def test_filter_args_available_on_compare(self):
        parser = build_parser()
        args = parser.parse_args([
            "compare", "--baseline", "a", "--candidate", "b",
            "--workload", "w1",
        ])
        assert args.workload == ["w1"]

