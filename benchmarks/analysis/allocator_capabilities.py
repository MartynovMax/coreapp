"""
allocator_capabilities — static capability descriptors for allocator experiments.

Describes what each allocator/arena supports so that workloads and reports can
query capabilities instead of hard-coding allocator-specific branches.

Usage::

    from analysis.allocator_capabilities import get_capabilities, ALLOCATOR_REGISTRY

    caps = get_capabilities("monotonic_arena")
    if caps.supports_bulk_reclaim:
        ...

    for name, caps in ALLOCATOR_REGISTRY.items():
        print(name, caps.kind)

    # All arenas that support rewind:
    from analysis.allocator_capabilities import list_allocators_with, AllocatorKind
    list_allocators_with(kind=AllocatorKind.ARENA, supports_rewind=True)
    # → ['monotonic_arena']
"""

from __future__ import annotations

from dataclasses import dataclass
from enum import Enum, auto
from typing import Dict, List, Optional


# ---------------------------------------------------------------------------
# Allocator kind taxonomy
# ---------------------------------------------------------------------------

class AllocatorKind(Enum):
    """High-level classification of an allocator or arena."""

    ALLOCATOR = auto()           # general-purpose allocator (malloc-style)
    ARENA = auto()               # region/arena — bulk-reclaim, no per-object free
    THREAD_LOCAL_ARENA = auto()  # arena with thread-local ownership semantics


# ---------------------------------------------------------------------------
# Capability descriptor
# ---------------------------------------------------------------------------

@dataclass(frozen=True)
class AllocatorCapabilities:
    """
    Immutable descriptor for a single allocator or arena used in the
    experiment suite.

    Fields
    ------
    name
        Canonical string identifier — matches the key in ALLOCATOR_REGISTRY
        and the ``allocator`` field emitted by C++ benchmark output.
    kind
        High-level classification (see AllocatorKind).
    supports_allocate
        True when the allocator exposes a per-object allocate call.
    supports_deallocate
        True when the allocator exposes a per-object deallocate/free call.
    supports_bulk_reclaim
        True when the allocator can release *all* live objects in O(1)
        (e.g. arena reset / destruction).
    supports_rewind
        True when the allocator exposes a marker/checkpoint API so that
        allocations made after the marker can be rewound in bulk.
    supports_variable_size
        True when the allocator accepts arbitrary byte sizes per request.
    optimized_for_fixed_size
        True when the allocator is specifically tuned for a single object
        size (e.g. a pool or freelist that pre-partitions fixed-size slots).
    thread_local_semantics
        True when the allocator assumes single-threaded / thread-local
        ownership — concurrent access from multiple threads is not supported
        or not safe without external synchronisation.
    """

    name: str
    kind: AllocatorKind

    # --- allocation interface -----------------------------------------------
    supports_allocate: bool
    supports_deallocate: bool
    supports_bulk_reclaim: bool
    supports_rewind: bool

    # --- size model ---------------------------------------------------------
    supports_variable_size: bool
    optimized_for_fixed_size: bool

    # --- threading model ----------------------------------------------------
    thread_local_semantics: bool


# ---------------------------------------------------------------------------
# Registry
# ---------------------------------------------------------------------------

#: Maps canonical allocator name → AllocatorCapabilities.
#: Add a new entry here whenever a new allocator joins the experiment suite.
ALLOCATOR_REGISTRY: Dict[str, AllocatorCapabilities] = {

    # ------------------------------------------------------------------
    # malloc_allocator
    # Thin wrapper around the system malloc/free.  Full interface, fully
    # general, no bulk-reclaim, no rewind.
    # ------------------------------------------------------------------
    "malloc_allocator": AllocatorCapabilities(
        name="malloc_allocator",
        kind=AllocatorKind.ALLOCATOR,
        supports_allocate=True,
        supports_deallocate=True,
        supports_bulk_reclaim=False,
        supports_rewind=False,
        supports_variable_size=True,
        optimized_for_fixed_size=False,
        thread_local_semantics=False,
    ),

    # ------------------------------------------------------------------
    # system_allocator
    # Direct OS-level allocator (e.g. VirtualAlloc / mmap).  Operates at
    # page granularity; individual deallocation is supported at the same
    # granularity.
    # ------------------------------------------------------------------
    "system_allocator": AllocatorCapabilities(
        name="system_allocator",
        kind=AllocatorKind.ALLOCATOR,
        supports_allocate=True,
        supports_deallocate=True,
        supports_bulk_reclaim=False,
        supports_rewind=False,
        supports_variable_size=True,
        optimized_for_fixed_size=False,
        thread_local_semantics=False,
    ),

    # ------------------------------------------------------------------
    # monotonic_arena
    # Bump-pointer arena.  Allocate-only; all memory is reclaimed at once
    # on reset/destruction.  Supports a rewind marker so a "stack frame"
    # of allocations can be undone cheaply.
    # ------------------------------------------------------------------
    "monotonic_arena": AllocatorCapabilities(
        name="monotonic_arena",
        kind=AllocatorKind.ARENA,
        supports_allocate=True,
        supports_deallocate=False,
        supports_bulk_reclaim=True,
        supports_rewind=True,
        supports_variable_size=True,
        optimized_for_fixed_size=False,
        thread_local_semantics=True,
    ),

    # ------------------------------------------------------------------
    # pool_allocator
    # Fixed-size slot pool.  O(1) alloc/dealloc; bulk reclaim on reset.
    # No variable-size support; heavily optimised for one object size.
    # ------------------------------------------------------------------
    "pool_allocator": AllocatorCapabilities(
        name="pool_allocator",
        kind=AllocatorKind.ALLOCATOR,
        supports_allocate=True,
        supports_deallocate=True,
        supports_bulk_reclaim=True,
        supports_rewind=False,
        supports_variable_size=False,
        optimized_for_fixed_size=True,
        thread_local_semantics=False,
    ),

    # ------------------------------------------------------------------
    # fallback_allocator
    # Two-level allocator: primary is tried first; on failure the request
    # falls through to a secondary (typically malloc).  Inherits the full
    # interface but adds the safety net of a general allocator.
    # ------------------------------------------------------------------
    "fallback_allocator": AllocatorCapabilities(
        name="fallback_allocator",
        kind=AllocatorKind.ALLOCATOR,
        supports_allocate=True,
        supports_deallocate=True,
        supports_bulk_reclaim=False,
        supports_rewind=False,
        supports_variable_size=True,
        optimized_for_fixed_size=False,
        thread_local_semantics=False,
    ),

    # ------------------------------------------------------------------
    # segregated_list_allocator
    # Size-class segregated free-list.  Variable-size interface; each
    # size class is backed by its own pool so fixed-size access paths
    # are fast.  Thread-safety depends on implementation; default
    # descriptor assumes shared use.
    # ------------------------------------------------------------------
    "segregated_list_allocator": AllocatorCapabilities(
        name="segregated_list_allocator",
        kind=AllocatorKind.ALLOCATOR,
        supports_allocate=True,
        supports_deallocate=True,
        supports_bulk_reclaim=False,
        supports_rewind=False,
        supports_variable_size=True,
        optimized_for_fixed_size=True,
        thread_local_semantics=False,
    ),

    # ------------------------------------------------------------------
    # freelist_allocator
    # Intrusive free-list over a fixed backing buffer.  Per-object alloc
    # and dealloc; no bulk reclaim; fixed-size slots only.
    # ------------------------------------------------------------------
    "freelist_allocator": AllocatorCapabilities(
        name="freelist_allocator",
        kind=AllocatorKind.ALLOCATOR,
        supports_allocate=True,
        supports_deallocate=True,
        supports_bulk_reclaim=False,
        supports_rewind=False,
        supports_variable_size=False,
        optimized_for_fixed_size=True,
        thread_local_semantics=False,
    ),

    # ------------------------------------------------------------------
    # thread_local_arena
    # Per-thread bump-pointer arena.  Each thread owns its region; no
    # cross-thread deallocation.  Bulk reclaim and rewind are available
    # within the owning thread.
    # ------------------------------------------------------------------
    "thread_local_arena": AllocatorCapabilities(
        name="thread_local_arena",
        kind=AllocatorKind.THREAD_LOCAL_ARENA,
        supports_allocate=True,
        supports_deallocate=False,
        supports_bulk_reclaim=True,
        supports_rewind=True,
        supports_variable_size=True,
        optimized_for_fixed_size=False,
        thread_local_semantics=True,
    ),
}


# ---------------------------------------------------------------------------
# Public helpers
# ---------------------------------------------------------------------------

def get_capabilities(allocator_name: str) -> AllocatorCapabilities:
    """
    Return the capability descriptor for *allocator_name*.

    Raises
    ------
    KeyError
        When *allocator_name* is not present in ALLOCATOR_REGISTRY.
        Callers that need a soft miss should use
        ``ALLOCATOR_REGISTRY.get(name)`` directly.
    """
    try:
        return ALLOCATOR_REGISTRY[allocator_name]
    except KeyError:
        known = ", ".join(sorted(ALLOCATOR_REGISTRY))
        raise KeyError(
            f"Unknown allocator {allocator_name!r}. "
            f"Known allocators: {known}"
        ) from None


def list_allocators_with(
    *,
    kind: Optional[AllocatorKind] = None,
    supports_allocate: Optional[bool] = None,
    supports_deallocate: Optional[bool] = None,
    supports_bulk_reclaim: Optional[bool] = None,
    supports_rewind: Optional[bool] = None,
    supports_variable_size: Optional[bool] = None,
    optimized_for_fixed_size: Optional[bool] = None,
    thread_local_semantics: Optional[bool] = None,
) -> List[str]:
    """
    Return a sorted list of allocator names whose capabilities match *all*
    supplied keyword filters.  Omitted (``None``) filters are not applied.

    Example::

        # All allocators that support bulk reclaim:
        list_allocators_with(supports_bulk_reclaim=True)
        # → ['monotonic_arena', 'pool_allocator', 'thread_local_arena']

        # Allocators safe for multi-threaded use that support deallocation:
        list_allocators_with(thread_local_semantics=False, supports_deallocate=True)
    """
    filters: Dict[str, object] = {}
    if kind is not None:
        filters["kind"] = kind
    if supports_allocate is not None:
        filters["supports_allocate"] = supports_allocate
    if supports_deallocate is not None:
        filters["supports_deallocate"] = supports_deallocate
    if supports_bulk_reclaim is not None:
        filters["supports_bulk_reclaim"] = supports_bulk_reclaim
    if supports_rewind is not None:
        filters["supports_rewind"] = supports_rewind
    if supports_variable_size is not None:
        filters["supports_variable_size"] = supports_variable_size
    if optimized_for_fixed_size is not None:
        filters["optimized_for_fixed_size"] = optimized_for_fixed_size
    if thread_local_semantics is not None:
        filters["thread_local_semantics"] = thread_local_semantics

    return sorted(
        name
        for name, caps in ALLOCATOR_REGISTRY.items()
        if all(getattr(caps, field) == value for field, value in filters.items())
    )

