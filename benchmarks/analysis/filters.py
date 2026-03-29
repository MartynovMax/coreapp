"""
filters.py — filtering support for RunModel data.

Responsibility:
  Produce filtered subsets of a RunModel based on one or more dimension
  criteria (allocator, workload, profile, experiment).

  Filtering operates on the canonical RunModel and returns a new RunModel
  that contains only the matching records.  The original RunModel is never
  mutated.

  All filters are optional.  Passing None for a dimension means "no filter
  for this dimension" (i.e. keep all values).  Passing an empty list is
  treated the same as None — it is not interpreted as "match nothing".

  Filtering is intentionally simple and explicit:
  - each dimension is an independent OR filter (any of the given values)
  - multiple dimensions are combined with AND (record must match all)

Public API:
  RunFilter                           — filter specification dataclass
  apply_filter(run, flt)              → RunModel  (filtered copy)
  filter_records(records, flt)        → list[SummaryRecord]  (low-level)
"""

from __future__ import annotations

from dataclasses import dataclass, field
from typing import Optional

from .models import RunModel, SummaryRecord


# ---------------------------------------------------------------------------
# Filter specification
# ---------------------------------------------------------------------------

@dataclass
class RunFilter:
    """
    Specifies which records to keep from a RunModel.

    Each dimension is either None/[] (no filter) or a list of accepted
    values.  An empty list is treated as "no filter" — not as "keep nothing".
    Matching is case-sensitive and exact.

    Attributes:
        allocators:   Keep records whose metadata.allocator is in this list.
        workloads:    Keep records whose workload field is in this list.
        profiles:     Keep records whose profile field is in this list.
        experiments:  Keep records whose metadata.experiment_name is in this list.
    """

    allocators:  Optional[list[str]] = field(default=None)
    workloads:   Optional[list[str]] = field(default=None)
    profiles:    Optional[list[str]] = field(default=None)
    experiments: Optional[list[str]] = field(default=None)

    def is_empty(self) -> bool:
        """Return True when no dimension has an active filter."""
        return all(
            not getattr(self, dim)
            for dim in ("allocators", "workloads", "profiles", "experiments")
        )


# ---------------------------------------------------------------------------
# Record-level predicate
# ---------------------------------------------------------------------------

def _matches(record: SummaryRecord, flt: RunFilter) -> bool:
    """
    Return True if the record satisfies all active filter dimensions.

    Each dimension uses OR semantics (value must appear in the allowed list).
    Dimensions that are None or [] are skipped (no restriction).
    Multiple active dimensions use AND semantics (all must match).
    """
    if flt.allocators:
        if record.metadata.allocator not in flt.allocators:
            return False

    if flt.experiments:
        if record.metadata.experiment_name not in flt.experiments:
            return False

    if flt.workloads:
        if record.workload not in flt.workloads:
            return False

    if flt.profiles:
        if record.profile not in flt.profiles:
            return False

    return True


# ---------------------------------------------------------------------------
# Public filtering helpers
# ---------------------------------------------------------------------------

def filter_records(
    records: list[SummaryRecord],
    flt: RunFilter,
) -> list[SummaryRecord]:
    """
    Return the subset of records that match the given filter.

    Preserves original ordering.  Does not mutate the input list.

    Args:
        records: Source list of SummaryRecord objects.
        flt:     Filter specification; None dimensions are ignored.

    Returns:
        A new list containing only the matching records.
    """
    if flt.is_empty():
        return list(records)
    return [r for r in records if _matches(r, flt)]


def apply_filter(run: RunModel, flt: RunFilter) -> RunModel:
    """
    Return a new RunModel whose summary records satisfy the given filter.

    The metadata and timeseries of the original run are carried over
    unchanged.  Only summary records are filtered because filtering
    dimensions (allocator, workload, profile, experiment) are defined
    on SummaryRecord fields.

    If the filter is empty, a shallow copy of the run is returned with
    the original summary list intact.

    Args:
        run: The source RunModel (not mutated).
        flt: Filter specification.

    Returns:
        A new RunModel with matching summary records only.
    """
    filtered_summary = filter_records(run.summary, flt)

    return RunModel(
        metadata=run.metadata,
        summary=filtered_summary,
        timeseries=run.timeseries,
    )

