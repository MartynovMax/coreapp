"""
analysis — offline analysis package for benchmark outputs.

C++ produces structured output files (.jsonl, .csv).
This package ingests, validates, assembles, reports, compares, and plots them.

Pipeline:
  ingest_summary / ingest_timeseries  →  validate  →  assemble
      →  filters  →  report / compare / plots

Modules:
  models              Internal domain dataclasses (RunModel, SummaryRecord, …)
  ingest_summary      Load summary.v2 CSV  → list[SummaryRecord]
  ingest_timeseries   Load ts.v2 JSONL     → list[TimeSeriesRecord]
  validate            Schema version and required-field checks
  assemble            Combine records into a canonical RunModel
  filters             RunFilter; apply_filter / filter_records
  report              Single-run Markdown report generation
  compare             Run-to-run NA-safe diff and Markdown output
  plots               Static chart generation (requires matplotlib)
  cli                 argparse entry point (report / compare / plot)
  ui                  Tkinter desktop UI (single run / compare / plots)

Entry point:
  python -m analysis <command> [options]
  python -m analysis --help
"""
