"""
cli.py — minimal command-line entry point for the analysis tool.

Usage:
    python -m analysis.cli report  --input <path> [filter options]
    python -m analysis.cli compare --baseline <path> --candidate <path> [filter options]
    python -m analysis.cli plot    --input <path> --kind <latency|throughput|timeseries>
                                   --output <path> [filter options]
                                   [--compare <path>]          # for throughput only
                                   [--metric-key <key>]        # for timeseries

Filter options (all optional, repeatable):
    --allocator  NAME   keep only records for this allocator (repeatable)
    --workload   NAME   keep only records for this workload  (repeatable)
    --profile    NAME   keep only records for this profile   (repeatable)
    --experiment NAME   keep only records for this experiment (repeatable)

This module is intentionally thin.  It is responsible for:
  - argument parsing
  - assembling RunModel(s) from file paths
  - building a RunFilter from CLI arguments
  - delegating to report.py / compare.py / plots.py

All business logic lives in report.py, compare.py, filters.py, plots.py,
and related modules.
"""

from __future__ import annotations

import argparse
import sys
import textwrap


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


def _cmd_plot(args: argparse.Namespace) -> int:
    from .assemble import assemble_run_from_files

    try:
        run = assemble_run_from_files(args.input, validate=True)
    except (FileNotFoundError, ValueError) as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1

    flt = _build_filter(args)

    from .plots import (
        plot_latency_summary,
        plot_throughput_comparison,
        plot_timeseries_trend,
    )

    try:
        if args.kind == "latency":
            from .filters import apply_filter
            effective = apply_filter(run, flt) if flt is not None else run
            plot_latency_summary(effective, args.output)
            print(f"wrote {args.output}")

        elif args.kind == "throughput":
            # Build a dict of labelled runs: the primary run plus any --compare runs.
            runs_by_label: dict[str, object] = {}

            primary_label = run.metadata.run_id or args.input
            from .filters import apply_filter
            runs_by_label[primary_label] = apply_filter(run, flt) if flt is not None else run

            for cmp_path in (args.compare or []):
                try:
                    cmp_run = assemble_run_from_files(cmp_path, validate=True)
                except (FileNotFoundError, ValueError) as exc:
                    print(f"error loading comparison run {cmp_path!r}: {exc}",
                          file=sys.stderr)
                    return 1
                label = cmp_run.metadata.run_id or cmp_path
                runs_by_label[label] = (
                    apply_filter(cmp_run, flt) if flt is not None else cmp_run
                )

            metric = args.metric_key or "ops_per_sec_mean"
            plot_throughput_comparison(runs_by_label, args.output, metric=metric)
            print(f"wrote {args.output}")

        elif args.kind == "timeseries":
            if not args.metric_key:
                print("error: --metric-key is required for --kind=timeseries",
                      file=sys.stderr)
                return 1
            # timeseries_filter restricts by experiment/allocator;
            # workload/profile dimensions are summary-only and are ignored.
            plot_timeseries_trend(run, args.metric_key, args.output,
                                  timeseries_filter=flt)
            print(f"wrote {args.output}")

        else:
            print(f"error: unknown plot kind {args.kind!r}", file=sys.stderr)
            return 1

    except ValueError as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1

    return 0


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        prog="python -m analysis",
        description="Offline analysis tool for coreapp benchmark outputs.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=textwrap.dedent("""\
            examples:
              # report for a single run
              python -m analysis report --input results/run1

              # compare two runs
              python -m analysis compare --baseline results/run1 --candidate results/run2

              # latency plot, filtered to one allocator
              python -m analysis plot --input results/run1 --kind latency \\
                      --output latency.png --allocator tlsf

              # throughput comparison across two runs
              python -m analysis plot --input results/run1 --compare results/run2 \\
                      --kind throughput --output throughput.png

              # time-series trend plot
              python -m analysis plot --input results/run1 --kind timeseries \\
                      --metric-key live_bytes --output live_bytes.png
        """),
    )
    subparsers = parser.add_subparsers(dest="command", metavar="<command>")
    subparsers.required = True

    # --- report ---
    report_parser = subparsers.add_parser(
        "report",
        help="Generate a Markdown report for a single benchmark run.",
        description=(
            "Load a benchmark run from <PATH>.csv (and optionally <PATH>.jsonl), "
            "validate it, and print a compact Markdown report to stdout."
        ),
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=textwrap.dedent("""\
            examples:
              python -m analysis report --input results/run1
              python -m analysis report --input results/run1 --allocator tlsf
              python -m analysis report --input results/run1 --experiment exp_mixed_sizes \\
                      --allocator tlsf --allocator mimalloc
        """),
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
        description=(
            "Load two runs from their respective base paths, validate both, "
            "and print a Markdown diff report to stdout.  Records are matched "
            "by (experiment_name, allocator, repetition index).  "
            "NA values are never treated as zero."
        ),
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=textwrap.dedent("""\
            examples:
              python -m analysis compare --baseline results/run1 --candidate results/run2
              python -m analysis compare --baseline results/run1 --candidate results/run2 \\
                      --allocator tlsf
              python -m analysis compare --baseline results/run1 --candidate results/run2 \\
                      --experiment exp_mixed_sizes --allocator tlsf --allocator mimalloc
        """),
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

    # --- plot ---
    plot_parser = subparsers.add_parser(
        "plot",
        help="Generate a static plot (PNG) from a benchmark run.",
        description=(
            "Load a run, optionally apply filters, and write a static chart to --output.\n\n"
            "Plot kinds:\n"
            "  latency     — grouped bar chart: min / median / p95 / max per record\n"
            "  throughput  — grouped bar chart: ops/sec per experiment across runs\n"
            "                (add --compare to include additional runs)\n"
            "  timeseries  — line chart: payload metric vs elapsed time from JSONL ticks\n"
            "                (requires --metric-key)"
        ),
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=textwrap.dedent("""\
            examples:
              python -m analysis plot --input results/run1 --kind latency --output latency.png
              python -m analysis plot --input results/run1 --kind latency --output latency.png \\
                      --allocator tlsf
              python -m analysis plot --input results/run1 --compare results/run2 \\
                      --kind throughput --output throughput.png
              python -m analysis plot --input results/run1 --kind timeseries \\
                      --metric-key live_bytes --output live_bytes.png
              python -m analysis plot --input results/run1 --kind timeseries \\
                      --metric-key live_bytes --output live_bytes.png \\
                      --allocator tlsf --experiment exp_mixed_sizes
        """),
    )
    plot_parser.add_argument(
        "--input",
        required=True,
        metavar="PATH",
        help="Base path of the primary run output (without extension).",
    )
    plot_parser.add_argument(
        "--kind",
        required=True,
        choices=["latency", "throughput", "timeseries"],
        metavar="KIND",
        help="Plot type: latency | throughput | timeseries.",
    )
    plot_parser.add_argument(
        "--output",
        required=True,
        metavar="PATH",
        help="Output image path (e.g. latency.png).",
    )
    plot_parser.add_argument(
        "--compare",
        dest="compare",
        action="append",
        default=[],
        metavar="PATH",
        help=(
            "Additional run base path to include in throughput comparison "
            "(repeatable, throughput only)."
        ),
    )
    plot_parser.add_argument(
        "--metric-key",
        dest="metric_key",
        default=None,
        metavar="KEY",
        help=(
            "Metric field for throughput (default: ops_per_sec_mean) "
            "or payload key for timeseries (required)."
        ),
    )
    _add_filter_args(plot_parser)
    plot_parser.set_defaults(func=_cmd_plot)

    return parser


def main() -> None:
    parser = build_parser()
    args = parser.parse_args()
    sys.exit(args.func(args))


if __name__ == "__main__":
    main()

