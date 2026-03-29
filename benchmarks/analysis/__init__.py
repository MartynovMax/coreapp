"""
analysis — offline analysis package for benchmark outputs.

C++ produces structured output files (.jsonl, .csv).
This package ingests, validates, assembles, reports, compares, and plots them.

Pipeline:
  ingest_summary / ingest_timeseries  →  validate  →  assemble  →  report / compare / plots
"""

