"""
cli.py — minimal command-line entry point for the analysis tool.

Usage:
    python -m analysis.cli report  --input <path>
    python -m analysis.cli compare --baseline <path> --candidate <path>

This module is intentionally thin.  It is responsible for:
  - argument parsing
  - assembling RunModel(s) from file paths
  - delegating to report.py / compare.py

All business logic lives in report.py, compare.py, and related modules.
"""

from __future__ import annotations

import argparse
import sys


def _cmd_report(args: argparse.Namespace) -> int:
    from .assemble import assemble_run_from_files

    try:
        run = assemble_run_from_files(args.input, validate=True)
    except (FileNotFoundError, ValueError) as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1

    # report logic is not implemented yet — hand off to report.py when ready
    print("report is not implemented yet")
    print(f"  run_id   : {run.metadata.run_id or '(unknown)'}")
    print(f"  summary  : {len(run.summary)} record(s)")
    print(f"  timeseries: {len(run.timeseries)} record(s)")
    return 0


def _cmd_compare(args: argparse.Namespace) -> int:
    from .assemble import assemble_run_from_files

    try:
        baseline  = assemble_run_from_files(args.baseline,  validate=True)
        candidate = assemble_run_from_files(args.candidate, validate=True)
    except (FileNotFoundError, ValueError) as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1

    # compare logic is not implemented yet — hand off to compare.py when ready
    print("compare is not implemented yet")
    print(f"  baseline  run_id : {baseline.metadata.run_id or '(unknown)'}")
    print(f"  candidate run_id : {candidate.metadata.run_id or '(unknown)'}")
    return 0


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        prog="analysis",
        description="Offline analysis tool for coreapp benchmark outputs.",
    )
    subparsers = parser.add_subparsers(dest="command", metavar="<command>")
    subparsers.required = True

    # --- report ---
    report_parser = subparsers.add_parser(
        "report",
        help="Generate a report from a single benchmark run.",
    )
    report_parser.add_argument(
        "--input",
        required=True,
        metavar="PATH",
        help="Base path of the run output (without extension).",
    )
    report_parser.set_defaults(func=_cmd_report)

    # --- compare ---
    compare_parser = subparsers.add_parser(
        "compare",
        help="Compare two benchmark runs and produce a diff report.",
    )
    compare_parser.add_argument(
        "--baseline",
        required=True,
        metavar="PATH",
        help="Base path of the baseline run output.",
    )
    compare_parser.add_argument(
        "--candidate",
        required=True,
        metavar="PATH",
        help="Base path of the candidate run output.",
    )
    compare_parser.set_defaults(func=_cmd_compare)

    return parser


def main() -> None:
    parser = build_parser()
    args = parser.parse_args()
    sys.exit(args.func(args))


if __name__ == "__main__":
    main()

