"""
cli.py — minimal command-line entry point for the analysis tool.

Usage:
    python -m analysis.cli report  --input <path> [filter options]
    python -m analysis.cli compare --baseline <path> --candidate <path> [filter options]

Filter options (all optional, repeatable):
    --allocator  NAME   keep only records for this allocator (repeatable)
    --workload   NAME   keep only records for this workload  (repeatable)
    --profile    NAME   keep only records for this profile   (repeatable)
    --experiment NAME   keep only records for this experiment (repeatable)

This module is intentionally thin.  It is responsible for:
  - argument parsing
  - assembling RunModel(s) from file paths
  - building a RunFilter from CLI arguments
  - delegating to report.py / compare.py

All business logic lives in report.py, compare.py, filters.py, and related modules.
"""

from __future__ import annotations

import argparse
import sys


# ---------------------------------------------------------------------------
# Filter helper
# ---------------------------------------------------------------------------

def _build_filter(args: argparse.Namespace):
    """Build a RunFilter from parsed CLI arguments, or None if no filters given."""
    from .filters import RunFilter

    flt = RunFilter(
        allocators=args.allocator  or None,
        workloads=args.workload    or None,
        profiles=args.profile      or None,
        experiments=args.experiment or None,
    )
    return None if flt.is_empty() else flt


def _add_filter_args(parser: argparse.ArgumentParser) -> None:
    """Add shared filter arguments to a subcommand parser."""
    parser.add_argument(
        "--allocator",
        dest="allocator",
        action="append",
        default=[],
        metavar="NAME",
        help="Filter to this allocator (repeatable).",
    )
    parser.add_argument(
        "--workload",
        dest="workload",
        action="append",
        default=[],
        metavar="NAME",
        help="Filter to this workload (repeatable).",
    )
    parser.add_argument(
        "--profile",
        dest="profile",
        action="append",
        default=[],
        metavar="NAME",
        help="Filter to this profile (repeatable).",
    )
    parser.add_argument(
        "--experiment",
        dest="experiment",
        action="append",
        default=[],
        metavar="NAME",
        help="Filter to this experiment (repeatable).",
    )


# ---------------------------------------------------------------------------
# Subcommand handlers
# ---------------------------------------------------------------------------

def _cmd_report(args: argparse.Namespace) -> int:
    from .assemble import assemble_run_from_files

    try:
        run = assemble_run_from_files(args.input, validate=True)
    except (FileNotFoundError, ValueError) as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1

    flt = _build_filter(args)

    from .report import print_run
    print_run(run, flt)
    return 0


def _cmd_compare(args: argparse.Namespace) -> int:
    from .assemble import assemble_run_from_files

    try:
        baseline  = assemble_run_from_files(args.baseline,  validate=True)
        candidate = assemble_run_from_files(args.candidate, validate=True)
    except (FileNotFoundError, ValueError) as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1

    flt = _build_filter(args)

    from .compare import compare_runs, print_comparison
    cmp = compare_runs(baseline, candidate, flt)
    print_comparison(cmp)
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
    _add_filter_args(report_parser)
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
    _add_filter_args(compare_parser)
    compare_parser.set_defaults(func=_cmd_compare)

    return parser


def main() -> None:
    parser = build_parser()
    args = parser.parse_args()
    sys.exit(args.func(args))


if __name__ == "__main__":
    main()

